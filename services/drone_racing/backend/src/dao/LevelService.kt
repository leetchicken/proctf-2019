package ae.hitb.proctf.drone_racing.dao

import org.jetbrains.exposed.dao.with
import org.jetbrains.exposed.sql.SortOrder


class LevelService {
    suspend fun getLevels(limit: Int=100): List<Level> = dbQuery {
        Level.all().orderBy(Levels.id to SortOrder.DESC).limit(limit).with(Level::author);
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