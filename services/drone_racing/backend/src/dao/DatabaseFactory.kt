package ae.hitb.proctf.drone_racing.dao

import com.typesafe.config.ConfigFactory
import com.zaxxer.hikari.HikariConfig
import com.zaxxer.hikari.HikariDataSource
import io.ktor.config.HoconApplicationConfig
import io.ktor.util.KtorExperimentalAPI
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.flywaydb.core.Flyway
import org.jetbrains.exposed.sql.Database
import org.jetbrains.exposed.sql.transactions.transaction

@KtorExperimentalAPI
class DatabaseFactory {
    private val appConfig = HoconApplicationConfig(ConfigFactory.load())
    private val dbConnectionString = appConfig.property("database.connectionString").getString()
    private val dbUser = appConfig.property("database.user").getString()
    private val dbPassword = appConfig.property("database.password").getString()

    fun init() {
        Database.connect(getDataSource())
        val flyway = Flyway.configure()
            .dataSource(dbConnectionString, dbUser, dbPassword)
            .locations("database/migrations")
            .baselineOnMigrate(true)
            .load()
        flyway.migrate()
    }

    private fun getDataSource(): HikariDataSource {
        val config = HikariConfig()
        config.driverClassName = "org.h2.Driver"
        config.jdbcUrl = dbConnectionString
        config.username = dbUser
        config.password = dbPassword
        config.maximumPoolSize = 50
        config.isAutoCommit = false
        config.transactionIsolation = "TRANSACTION_REPEATABLE_READ"
        config.validate()
        return HikariDataSource(config)
    }
}

suspend fun <T> dbQuery(block: () -> T): T =
    withContext(Dispatchers.IO) {
        transaction { block() }
    }
