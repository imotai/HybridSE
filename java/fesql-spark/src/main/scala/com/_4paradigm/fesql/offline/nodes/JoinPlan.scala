package com._4paradigm.fesql.offline.nodes

import com._4paradigm.fesql.`type`.TypeOuterClass.ColumnDef
import com._4paradigm.fesql.codec.RowView
import com._4paradigm.fesql.node.JoinType
import com._4paradigm.fesql.offline._
import com._4paradigm.fesql.offline.utils.{FesqlUtil, SparkColumnUtil, SparkRowUtil}
import com._4paradigm.fesql.vm.{CoreAPI, FeSQLJITWrapper, PhysicalJoinNode}
import org.apache.spark.broadcast.Broadcast
import org.apache.spark.sql.catalyst.encoders.RowEncoder
import org.apache.spark.sql.types.{LongType, StructType}
import org.apache.spark.sql.{Column, Row, functions}

import scala.collection.mutable


object JoinPlan {

  def gen(ctx: PlanContext, node: PhysicalJoinNode, left: SparkInstance, right: SparkInstance): SparkInstance = {
    val joinType = node.getJoin_type_
    if (joinType != JoinType.kJoinTypeLeft && joinType != JoinType.kJoinTypeLast) {
      throw new FeSQLException(s"Join type $joinType not supported")
    }

    val sess = ctx.getSparkSession

    val indexName = "__JOIN_INDEX__-" + System.currentTimeMillis()
    val leftDf = {
      if (node.getJoin_type_ == JoinType.kJoinTypeLast) {
        val indexedRDD = left.getRDD.zipWithIndex().map {
          case (row, id) => Row.fromSeq(row.toSeq :+ id)
        }
        sess.createDataFrame(indexedRDD,
          left.getSchema.add(indexName, LongType))

      } else {
        left.getDf(sess)
      }
    }

    val rightDf = right.getDf(sess)

    val inputSchemaSlices = FesqlUtil.getOutputSchemaSlices(node)

    // get left index
    val leftKeys = node.join().left_key().keys()
    val leftKeyCols = mutable.ArrayBuffer[Column]()
    for (i <- 0 until leftKeys.GetChildNum()) {
      val expr = leftKeys.GetChild(i)
      val (_, _, column) = SparkColumnUtil.resolveColumn(expr, leftDf, ctx)
      leftKeyCols += column
    }

    // get right index
    val rightKeys = node.join().right_key().keys()
    val rightKeyCols = mutable.ArrayBuffer[Column]()
    for (i <- 0 until rightKeys.GetChildNum()) {
      val expr = rightKeys.GetChild(i)
      val (_, _, column) = SparkColumnUtil.resolveColumn(expr, rightDf, ctx)
      rightKeyCols += column
    }

    // build join condition
    val joinConditions = mutable.ArrayBuffer[Column]()
    if (leftKeyCols.lengthCompare(rightKeyCols.length) != 0) {
      throw new FeSQLException("Illegal join key conditions")
    }

    // == key conditions
    for ((leftCol, rightCol) <- leftKeyCols.zip(rightKeyCols)) {
      // TODO: handle null value
      joinConditions += leftCol === rightCol
    }

    val filter = node.join().filter()
    // extra conditions
    if (filter.condition() != null) {

      val regName = "FESQL_JOIN_CONDITION_" + node.GetFnName()
      val conditionUDF = new JoinConditionUDF(
        functionName = filter.fn_info().fn_name(),
        inputSchemaSlices = inputSchemaSlices,
        outputSchema = filter.fn_info().fn_schema(),
        moduleTag = ctx.getTag,
        moduleBroadcast = ctx.getModuleBufferBroadcast
      )
      sess.udf.register(regName, conditionUDF)

      val allColWrap = functions.struct(
          leftDf.columns.filter(_ != indexName).map(leftDf.col) ++
          rightDf.columns.map(rightDf.col)
      : _*)
      joinConditions += functions.callUDF(regName, allColWrap)
    }

    if (joinConditions.isEmpty) {
      throw new FeSQLException("No join conditions specified")
    }

    var joined = leftDf.join(rightDf, joinConditions.reduce(_ && _),  "left")

    // column renaming
    val leftName = left.getName
    val rightName = right.getName
    val renameCols = {
      leftDf.columns.map(n => {
        if (n == indexName) {
          leftDf.col(n)
        } else {
          leftDf.col(n).alias(leftName + "." + n)
        }
      }) ++
      rightDf.columns.map(n => {
        rightDf.col(n).alias(rightName + "." + n)
      })
    }
    joined = joined.select(renameCols: _*)

    val result = if (node.join_type() == JoinType.kJoinTypeLast) {
      val indexColIdx = leftDf.schema.size - 1

      val (_, timeColIdx, _) = SparkColumnUtil.resolveColumn(
        node.join().right_key().keys().GetChild(0), rightDf, ctx)

      val timeColType = rightDf.schema(timeColIdx).dataType
      val timeIdxInJoined = timeColIdx + leftDf.schema.size

      import sess.implicits._

      val distinct = joined
        .groupByKey {
          row => row.getLong(indexColIdx)
        }
        .mapGroups {
          case (_, iter) =>
            val timeExtractor = SparkRowUtil.createOrderKeyExtractor(
              timeIdxInJoined, timeColType, nullable=false)

            iter.maxBy(row => timeExtractor.apply(row))

        }(RowEncoder(joined.schema))

      distinct.drop(indexName)

    } else {
      joined
    }

    SparkInstance.fromDataFrame(null, result)
  }


  class JoinConditionUDF(functionName: String,
                         inputSchemaSlices: Array[StructType],
                         outputSchema: java.util.List[ColumnDef],
                         moduleTag: String,
                         moduleBroadcast: Broadcast[SerializableByteBuffer]
                        ) extends Function1[Row, Boolean] with Serializable {

    @transient private lazy val tls = new ThreadLocal[UnSafeJoinConditionUDFImpl]() {
      override def initialValue(): UnSafeJoinConditionUDFImpl = {
        new UnSafeJoinConditionUDFImpl(
          functionName, inputSchemaSlices, outputSchema, moduleTag, moduleBroadcast)
      }
    }

    override def apply(row: Row): Boolean = {
      tls.get().apply(row)
    }
  }

  class UnSafeJoinConditionUDFImpl(functionName: String,
                                   inputSchemaSlices: Array[StructType],
                                   outputSchema: java.util.List[ColumnDef],
                                   moduleTag: String,
                                   moduleBroadcast: Broadcast[SerializableByteBuffer]
                                  ) extends Function1[Row, Boolean] with Serializable {
    private val jit = initJIT()

    private val fn: Long = jit.FindFunction(functionName)
    if (fn == 0) {
      throw new FeSQLException(s"Fail to find native jit function $functionName")
    }

    // TODO: these are leaked
    private val bufferPool: NativeBufferPool = new NativeBufferPool
    private val encoder: SparkRowCodec = new SparkRowCodec(inputSchemaSlices, bufferPool)
    private val outView: RowView = new RowView(outputSchema)

    def initJIT(): FeSQLJITWrapper = {
      // ensure worker native
      val buffer = moduleBroadcast.value.getBuffer
      JITManager.initJITModule(moduleTag, buffer)
      JITManager.getJIT(moduleTag)
    }

    override def apply(row: Row): Boolean = {
      // call encode
      val nativeInputRow = encoder.encode(row, keepBuffer=false)

      // call native compute
      val result = CoreAPI.ComputeCondition(fn, nativeInputRow, outView, 0)

      // release swig jni objects
      nativeInputRow.delete()

      result
    }
  }
}