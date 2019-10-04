package ae.hitb.proctf.drone_racing.dao

import org.jetbrains.exposed.dao.with


class LevelService {
    suspend fun getLevels(): List<Level> = dbQuery {
        Level.all().with(Level::author);
    }

    suspend fun findLevelById(id: Int) = dbQuery {
        Level.findById(id)
    }

    suspend fun createLevel(author: User, title: String, map: String) = dbQuery {
        Level.new {
            this.author = author
            this.title = title
            this.map = map
        }
    }
}