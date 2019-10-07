package ae.hitb.proctf.drone_racing.programming.language

sealed class Expression

data class Variable(val name: String) : Expression()

data class Param(val name: String): Expression()

data class IntLiteral(val value: Int) : Expression()
data class StringLiteral(val value: String) : Expression()

data class FunctionCall(val functionDeclaration: FunctionDeclaration,
                        val argumentExpressions: List<Expression>) : Expression() {
    init { require(argumentExpressions.size == functionDeclaration.parameterNames.size) }
}

data class UnaryOperation(val operand: Expression, val kind: UnaryOperationKind) : Expression()
sealed class UnaryOperationKind
object Not : UnaryOperationKind()

data class BinaryOperation(val left: Expression, val right: Expression, val kind: BinaryOperationKind) : Expression()
sealed class BinaryOperationKind
object Plus : BinaryOperationKind()
object Minus : BinaryOperationKind()
object Times : BinaryOperationKind()
object Div : BinaryOperationKind()
object Rem : BinaryOperationKind()
object And : BinaryOperationKind()
object Or : BinaryOperationKind()
object Eq : BinaryOperationKind()
object Neq : BinaryOperationKind()
object Gt : BinaryOperationKind()
object Lt : BinaryOperationKind()
object Leq : BinaryOperationKind()
object Geq : BinaryOperationKind()
