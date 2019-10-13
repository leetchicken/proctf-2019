package ae.hitb.proctf.drone_racing.dao

import com.google.gson.annotations.JsonAdapter
import com.rnett.exposedgson.ExposedGSON
import com.rnett.exposedgson.ExposedTypeAdapter
import org.jetbrains.exposed.dao.*

const val PROGRAM_MAX_SIZE = 50000 // bytes
const val PROGRAM_TITLE_MAX_SIZE = 46

object Programs : IntIdTable() {
    val author = reference("author_id", Users)
    val level = reference("level_id", Levels)
    val title = varchar("title", 100)
    val file = varchar("file", 100)
}

@JsonAdapter(ExposedTypeAdapter::class)
@ExposedGSON.JsonDatabaseIdField("id")
class Program(id: EntityID<Int>) : IntEntity(id) {
    companion object : IntEntityClass<Program>(Programs)

    var author by User referencedOn Programs.author
    @ExposedGSON.Ignore
    var level by Level referencedOn Programs.level
    var title by Programs.title
    @ExposedGSON.Ignore
    var file by Programs.file

    override fun toString(): String {
        return "Program[$title by $author for $level]"
    }
}
