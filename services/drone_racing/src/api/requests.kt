package ae.hitb.proctf.drone_racing.api

data class CreateUserRequest(val name: String, val login: String, val password: String)
data class LoginRequest(val login: String, val password: String)

data class CreateLevelRequest(val title: String, val height: Int, val width: Int, val map: String)

data class CreateProgramRequest(val levelId: Int, val title: String, val sourceCode: String)

data class CreateRunRequest(val programId: Int, val params: Map<String, String>)