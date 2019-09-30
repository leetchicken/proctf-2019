package ae.hitb.proctf.drone_racing.dao

import com.google.gson.annotations.JsonAdapter
import com.rnett.exposedgson.ExposedGSON
import com.rnett.exposedgson.ExposedTypeAdapter
import org.jetbrains.exposed.dao.*

object Runs : IntIdTable() {
    val program = reference("program_id", Programs)
    val level = reference("level_id", Levels)
    val startTime = integer("start_time")
    val finishTime = integer("finish_time").nullable()
    val success = bool("success").default(false)
    val score = integer("score").default(0)
}

@JsonAdapter(ExposedTypeAdapter::class)
@ExposedGSON.JsonDatabaseIdField("id")
class Run(id: EntityID<Int>) : IntEntity(id) {
    companion object : IntEntityClass<Run>(Runs)

    @ExposedGSON.Ignore
    var program by Program referencedOn Runs.program
    @ExposedGSON.Ignore
    var level by Level referencedOn Runs.level
    var startTime by Runs.startTime
    var finishTime by Runs.finishTime
    var success by Runs.success
    var score by Runs.score

    val isFinished get() = finishTime != null;

    override fun toString(): String {
        return "Run[$program: $startTime → $finishTime. Success: $success]"
    }
}
