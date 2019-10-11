package ae.hitb.proctf.drone_racing.dao

import com.google.gson.annotations.JsonAdapter
import com.rnett.exposedgson.ExposedGSON
import com.rnett.exposedgson.ExposedTypeAdapter
import org.jetbrains.exposed.dao.*

object Users : IntIdTable() {
    val login = varchar("login", 100).uniqueIndex()
    val passwordHash = varchar("password_hash", 60)
    val name = varchar("name", 100)
}

@JsonAdapter(ExposedTypeAdapter::class)
@ExposedGSON.JsonDatabaseIdField("id")
class User(id: EntityID<Int>) : IntEntity(id) {
    companion object : IntEntityClass<User>(Users)

    var login by Users.login
    @ExposedGSON.Ignore
    var passwordHash by Users.passwordHash
    var name by Users.name

    override fun toString(): String {
        return "User[$login → $name]"
    }
}