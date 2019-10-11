package ae.hitb.proctf.drone_racing.programming.language

operator fun Expression.plus(other: Expression) = BinaryOperation(this, other, Plus)
operator fun Expression.minus(other: Expression) = BinaryOperation(this, other, Minus)
operator fun Expression.times(other: Expression) = BinaryOperation(this, other, Times)
operator fun Expression.div(other: Expression) = BinaryOperation(this, other, Div)
operator fun Expression.rem(other: Expression) = BinaryOperation(this, other, Rem)
