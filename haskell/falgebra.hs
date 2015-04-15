
newtype Fix f = Fx (f (Fix f))

data ExprF a = Const Int | Add a a | Mul a a
type Expr = Fix ExprF

instance Functor ExprF where
  fmap eval (Const i) = Const i
  fmap eval (left `Add` right) = (eval left) `Add` (eval right)
  fmap eval (left `Mul` right) = (eval left) `Mul` (eval right)

main = return 0

