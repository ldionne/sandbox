import Control.Monad
import Data.Foldable
import Control.Applicative


-- Comonad
class Functor w => Comonad w where
  extract :: w a -> a

  duplicate :: w a -> w (w a)
  duplicate = extend id

  extend :: (w a -> b) -> w a -> w b
  extend f = fmap f . duplicate

data Lazy2 a = Lazy2 (() -> a)

eval :: Lazy2 a -> a
eval (Lazy2 lx) = lx ()

instance Functor Lazy2 where
  -- fmap :: (a -> b) -> Lazy2 a -> Lazy2 b
  fmap f lx = Lazy2 (\_ -> f (eval lx))

instance Comonad Lazy2 where
  -- extract :: Lazy2 a -> a
  extract = eval

  -- duplicate :: Lazy2 a -> Lazy2 (Lazy2 a)
  duplicate lx = Lazy2 (\_ -> lx)

  -- extend :: (Lazy2 a -> b) -> Lazy2 a -> Lazy2 b
  extend f lx = Lazy2 (\_ -> f lx)


-- Lazy
data Lazy a b = Lazy (a -> b) a

instance Functor (Lazy a) where
  -- fmap :: (b -> c) -> (Lazy a b) -> (Lazy a c)
  fmap g (Lazy f x) = Lazy (g . f) x


{-

First law: `extract . duplicate = id`
  (extract . duplicate) (Lazy f x) = extract (duplicate (Lazy f x))
                                   = extract (Lazy (const (Lazy f x)) undefined)
                                   = (const (Lazy f x)) undefined
                                   = Lazy f x

Second law: `fmap extract . duplicate = id`
  (fmap extract . duplicate) (Lazy f x) = fmap extract (duplicate (Lazy f x))
                                        = fmap extract (Lazy (const (Lazy f x)) undefined)
                                        = Lazy (extract . (const (Lazy f x))) undefined
                                        = Lazy f x              -- see (*)

(*)
  extract (Lazy (extract . (const (Lazy f x))) undefined) = (extract . (const (Lazy f x))) undefined
                                                          = extract $ const (Lazy f x) undefined
                                                          = extract (Lazy f x)
                                                          = f x

Third law: `duplicate . duplicate = fmap duplicate . duplicate`
  (duplicate . duplicate) (Lazy f x) = duplicate (duplicate (Lazy f x))
                                     = duplicate $ Lazy (const (Lazy f x)) undefined
                                     = Lazy (const (Lazy (const (Lazy f x)) undefined)) undefined
                                     = -- see (**)

  (fmap duplicate . duplicate) (Lazy f x) = fmap duplicate $ duplicate (Lazy f x)
                                          = fmap duplicate (Lazy (const (Lazy f x)) undefined)
                                          = Lazy (duplicate . const (Lazy f x)) undefined
                                          = -- see (***)
(**)
  extract Lazy (const (Lazy (const (Lazy f x)) undefined)) undefined = const (Lazy (const (Lazy f x)) undefined) undefined
                                                                     = Lazy (const (Lazy f x)) undefined
                                                                     = duplicate (Lazy f x)

(***)
  extract (Lazy (duplicate . const (Lazy f x)) undefined) = (duplicate . const (Lazy f x)) undefined
                                                          = duplicate (const (Lazy f x) undefined)
                                                          = duplicate (Lazy f x)

-}
instance Comonad (Lazy a) where
  -- extract :: Lazy a b -> b
  extract (Lazy f x) = f x

  -- duplicate :: Lazy a b -> Lazy a (Lazy a b)
  duplicate (Lazy f x) = Lazy (const (Lazy f x)) undefined

  -- extend :: (Lazy a b -> c) -> Lazy a b -> Lazy a c
  extend g (Lazy f x) = Lazy (const (g (Lazy f x))) undefined


lazy_value v = Lazy id v


{-
instance Applicative (Lazy a) where
  -- pure :: b -> Lazy a b
  pure x = Lazy (const x) undefined

  -- <*> :: Lazy a (b -> c) -> Lazy a b -> Lazy a c
  (Lazy f x) <*> (Lazy g y) = pure $ (f x) (g y)
-}


-- Identity
data Identity a = Identity a deriving Show

instance Functor Identity where
  fmap f (Identity x) = Identity (f x)

instance Applicative Identity where
  pure = Identity
  (Identity f) <*> (Identity x) = Identity (f x)

instance Comonad Identity where
  extract (Identity x) = x
  duplicate (Identity x) = Identity (Identity x)
  extend f (Identity x) = Identity (f (Identity x))



-- Product
data Product e a = Product e a deriving Show

instance Functor (Product e) where
  fmap f (Product e a) = Product e (f a)

instance Comonad (Product e) where
  extract (Product e a) = a
  duplicate (Product e a) = Product e (Product e a)


-- Logical
class Logical b where
    if_ :: (Comonad w) => w b -> w x -> w x -> w x

    and_ :: (Comonad w) => w b -> w b -> w b
    and_ x y = if_ x y x

    or_ :: (Comonad w) => w b -> w b -> w b
    or_ x y = if_ x x y

    eval_if :: (Comonad w) => w b -> w x -> w x -> x
    eval_if c t e = extract $ if_ c t e

instance Logical Bool where
    if_ c t e = if (extract c) then t else e




main = return 1



{-
zzzz213 = if_ (lazy_value True)
        (Lazy $ eval_if (lazy_value True)
            (Lazy 1)
            (Lazy 2))
        (Lazy 3)
-}

{-
chain :: Comonad w => (w a -> w b) -> (b -> w c) -> ()

chain :: Comonad w => w x -> (x -> w y) -> (w x -> w y)
chain f g = \wx -> f (extract wx)

if_ :: (Comonad w)  => w Bool -> w x -> w x -> w x
if_ c t e = if (extract c) then t else e

z = if_ (Lazy True) (Lazy 1) (Lazy 2)
-}
