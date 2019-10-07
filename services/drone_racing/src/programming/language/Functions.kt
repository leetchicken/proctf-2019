package ae.hitb.proctf.drone_racing.programming.language

import java.util.*

enum class FunctionType {
    INTEGER,
    STRING,
    VOID,
}

open class FunctionDeclaration(
    open val name: String,
    val parameterNames: List<Variable>,
    val parameterTypes: List<FunctionType>,
    val returnType: FunctionType,
    open val body: Statement
) {
    override fun hashCode(): Int = Objects.hash(name, parameterNames)
    override fun equals(other: Any?) =
            other is FunctionDeclaration && other.name == name && other.parameterNames == parameterNames

    override fun toString(): String = "$name(${parameterNames.zip(parameterTypes).map { it.first.name + ": " + it.second.name.toLowerCase() }.joinToString()}): " + returnType.name.toLowerCase()
}

data class UnresolvedFunction(override val name: String, val dimensions: Int) : FunctionDeclaration(name, (1..dimensions).map { Variable("unresolved") }, (1..dimensions).map{ FunctionType.STRING }, FunctionType.VOID, Pass) {
    override val body get() = throw IllegalStateException("Getting body of an unresolved function $this")
}

sealed class Intrinsic(name: String, parameterNames: List<Variable>, parameterTypes: List<FunctionType>, returnType: FunctionType) : FunctionDeclaration(name, parameterNames, parameterTypes, returnType, Pass) {
    override val body: Statement get() = throw IllegalStateException("Getting body of an unresolved function $this")

    object READ : Intrinsic("read", emptyList(), emptyList(), FunctionType.VOID)
    object WRITE : Intrinsic("write", listOf(Variable("expression")), listOf(FunctionType.INTEGER), FunctionType.VOID)
    object WRITESTRING : Intrinsic("write_string", listOf(Variable("expression")), listOf(FunctionType.STRING), FunctionType.VOID)
    object STRMAKE : Intrinsic("strmake", listOf(Variable("n"), Variable("c")), listOf(FunctionType.INTEGER, FunctionType.STRING), FunctionType.STRING)
    object STRCMP : Intrinsic("strcmp", listOf(Variable("S1"), Variable("S2")), listOf(FunctionType.STRING, FunctionType.STRING), FunctionType.INTEGER)
    object STRGET : Intrinsic("strget", listOf(Variable("S"), Variable("i")), listOf(FunctionType.STRING, FunctionType.INTEGER), FunctionType.INTEGER)
    object STRDUP : Intrinsic("strdup", listOf(Variable("S")), listOf(FunctionType.STRING), FunctionType.STRING)
    object STRSET : Intrinsic("strset", listOf(Variable("S"), Variable("i"), Variable("c")), listOf(FunctionType.STRING, FunctionType.INTEGER, FunctionType.INTEGER), FunctionType.VOID)
    object STRCAT : Intrinsic("strcat", listOf(Variable("S1"), Variable("S2")), listOf(FunctionType.STRING, FunctionType.STRING), FunctionType.STRING)
    object STRSUB : Intrinsic("strsub", listOf(Variable("S"), Variable("i"), Variable("j")), listOf(FunctionType.STRING, FunctionType.INTEGER, FunctionType.INTEGER), FunctionType.STRING)
    object STRLEN : Intrinsic("strlen", listOf(Variable("S")), listOf(FunctionType.STRING), FunctionType.INTEGER)

    companion object {
        val resolvable by lazy {
            listOf(
                READ, WRITE, WRITESTRING, STRMAKE, STRCMP, STRGET, STRDUP, STRSET, STRCAT, STRSUB, STRLEN
            )
        }
    }
}