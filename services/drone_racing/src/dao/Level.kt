package ae.hitb.proctf.drone_racing.dao

import com.google.gson.annotations.JsonAdapter
import com.rnett.exposedgson.ExposedGSON
import com.rnett.exposedgson.ExposedTypeAdapter
import org.jetbrains.exposed.dao.EntityID
import org.jetbrains.exposed.dao.IntEntity
import org.jetbrains.exposed.dao.IntEntityClass
import org.jetbrains.exposed.dao.IntIdTable

const val LEVEL_MAX_SIZE = 50

object Levels : IntIdTable() {
    val author = reference("author_id", Users)
    val title = varchar("title", 100)
    val height = integer("height")
    val width = integer("width")
    val map = varchar("map", LEVEL_MAX_SIZE * LEVEL_MAX_SIZE)
}

@JsonAdapter(ExposedTypeAdapter::class)
@ExposedGSON.JsonDatabaseIdField("id")
class Level(id: EntityID<Int>) : IntEntity(id) {
    companion object : IntEntityClass<Level>(Levels)

    @ExposedGSON.Ignore
    var author by User referencedOn Levels.author
    var title by Levels.title
    var height by Levels.height
    var width by Levels.width
    var map by Levels.map

    override fun toString(): String {
        return "Level[$title, $width x $height]"
    }
}
