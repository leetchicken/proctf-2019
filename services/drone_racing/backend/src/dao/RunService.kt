package ae.hitb.proctf.drone_racing.dao

import org.jetbrains.exposed.dao.with
import org.jetbrains.exposed.sql.SortOrder
import org.jetbrains.exposed.sql.select

class RunService {
    suspend fun findRunById(id: Int) = dbQuery {
        Run.findById(id)
    }

    fun findRunsOnLevel(level: Level, limit: Int=100): List<Run> {
        val query = Runs.innerJoin(Programs).select {
            Programs.level eq level.id
        }.withDistinct()

        return Run.wrapRows(query).orderBy(Runs.score to SortOrder.ASC).limit(limit).with(Run::program).with(Program::author).toList()
    }

    fun createRun(program: Program, startTime: Long, finishTime: Long, success: Boolean, score: Int): Run {
        return Run.new {
            this.program = program
            this.level = program.level
            this.startTime = startTime
            this.finishTime = finishTime
            this.success = success
            this.score = score
        }
    }
}