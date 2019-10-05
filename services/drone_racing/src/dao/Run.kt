package ae.hitb.proctf.drone_racing.dao

import com.google.gson.annotations.JsonAdapter
import com.rnett.exposedgson.ExposedGSON
import com.rnett.exposedgson.ExposedTypeAdapter
import org.jetbrains.exposed.dao.*

object Runs : IntIdTable() {
    val program = reference("program_id", Programs)
    val level = reference("level_id", Levels)
    val startTime = long("start_time")
    val finishTime = long("finish_time")
    val success = bool("success").default(false)
    val score = integer("score").default(0)
}

@JsonAdapter(ExposedTypeAdapter::class)
@ExposedGSON.JsonDatabaseIdField("id")
class Run(id: EntityID<Int>) : IntEntity(id) {
    companion object : IntEntityClass<Run>(Runs)

    var program by Program referencedOn Runs.program
    @ExposedGSON.Ignore
    var level by Level referencedOn Runs.level
    var startTime by Runs.startTime
    var finishTime by Runs.finishTime
    var success by Runs.success
    var score by Runs.score

    override fun toString(): String {
        return "Run[$program: $startTime → $finishTime. Success: $success]"
    }
}
