package ae.hitb.proctf.drone_racing.dao

class RunService {
    suspend fun findRunById(id: Int) = dbQuery {
        Run.findById(id)
    }

    suspend fun findRunsOnLevel(level: Level) = dbQuery {
        Program.find { Programs.level eq level.id }.toList()
    }

    suspend fun createRun(program: Program, startTime: Int, finishTime: Int?, success: Boolean, score: Int) = dbQuery {
        Run.new {
            this.program = program
            this.level = program.level
            this.startTime = startTime
            this.finishTime = finishTime
            this.success = success
            this.score = score
        }
    }
}