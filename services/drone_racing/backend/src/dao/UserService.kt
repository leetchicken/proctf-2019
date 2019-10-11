package ae.hitb.proctf.drone_racing.dao

import org.springframework.security.crypto.password.PasswordEncoder

class UserService(private val passwordEncoder: PasswordEncoder) {
    suspend fun getUsers() = dbQuery {
        User.all().toList()
    }

    suspend fun findUserById(id: Int) = dbQuery {
        User.findById(id)
    }

    suspend fun findUserByLogin(login: String) = dbQuery {
        User.find { Users.login eq login }.firstOrNull()
    }

    suspend fun authenticate(login: String, password: String): User? {
        val user = findUserByLogin(login) ?: return null
        if (! passwordEncoder.matches(password, user.passwordHash))
            return null
        return user
    }

    suspend fun createUser(name: String, login: String, password: String) = dbQuery {
        User.new {
            this.login = login
            this.passwordHash = passwordEncoder.encode(password)
            this.name = name
        }
    }
}