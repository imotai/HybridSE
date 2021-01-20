package com._4paradigm.fesql.spark.nodes.window

import com._4paradigm.fesql.spark.FeSQLConfig
import com._4paradigm.fesql.spark.nodes.WindowAggPlan.WindowAggConfig
import org.apache.spark.sql.Row
import org.slf4j.LoggerFactory


class RowDebugger(sqlConfig: FeSQLConfig, config: WindowAggConfig, isSkew: Boolean) extends WindowHook {

  private val logger = LoggerFactory.getLogger(this.getClass)

  private val sampleInterval = sqlConfig.printSampleInterval
  private var cnt: Long = 0

  override def preCompute(computer: WindowComputer, curRow: Row): Unit = {
    printRow(computer, curRow)
  }

  override def preBufferOnly(computer: WindowComputer, curRow: Row): Unit = {
    printRow(computer, curRow)
  }

  def printRow(computer: WindowComputer, row: Row): Unit = {
    if (cnt % sampleInterval == 0) {
      val str = new StringBuffer()
      str.append(row.get(config.orderIdx))
      str.append(",")
      for (e <- config.groupIdxs) {
        str.append(row.get(e))
        str.append(",")
      }
      str.append(" window size = " + computer.getWindow.size())
      if (isSkew) {
        val tag = row.getInt(config.skewTagIdx)
        val position = row.getInt(config.skewPositionIdx)
        logger.info(s"tag : postion = $tag : $position " +
          s"threadId = ${Thread.currentThread().getId} " +
          s"cnt = $cnt rowInfo = ${str.toString}")
      } else {
        logger.info(s"threadId = ${Thread.currentThread().getId}" +
          s"cnt = $cnt rowInfo = ${str.toString}")
      }
    }
    cnt += 1
  }
}