package ae.hitb.proctf.drone_racing.programming.language

sealed class Statement

object Pass : Statement()
data class AssignStatement(val variable: Variable, val expression: Expression) : Statement()
data class IfStatement(val condition: Expression,
                       val trueBranch: Statement,
                       val falseBranch: Statement
) : Statement()

data class WhileStatement(val condition: Expression,
                          val body: Statement
) : Statement()

data class ChainStatement(val leftPart: Statement,
                          val rightPart: Statement
) : Statement()

data class ReturnStatement(val expression: Expression) : Statement()

data class FunctionCallStatement(val functionCall: FunctionCall) : Statement()

fun chainOf(vararg statements: Statement) = statements.reduce(::ChainStatement)

// Exceptions

data class ExceptionType(val name: String)

data class CatchBranch(val exceptionType: ExceptionType, val dataVariable: Variable, val body: Statement)

data class TryStatement(
    val body: Statement,
    val catchBranches: List<CatchBranch>,
    val finallyStatement: Statement = Pass
) : Statement()

data class ThrowStatement(val exceptionType: ExceptionType, val dataExpression: Expression) : Statement()