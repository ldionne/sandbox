{-# LANGUAGE InstanceSigs #-}

import Prelude hiding (foldr, foldl)
import Control.Monad
import Data.Foldable
import qualified Data.Map
import Control.Applicative
import Data.Traversable
import Data.Functor.Identity

instance Show a => Show (Identity a) where
    show x = "Identity " ++ show (runIdentity x)

------------------------------------------------------------------------------

adjust :: Functor f => (x -> Bool) -> (x -> x) -> f x -> f x
adjust p f xs = fmap (\x -> if p x then f x else x) xs

class Product p where
    first :: p x y -> x
    second :: p x y -> y


data Pair x y = Pair x y
instance Product Pair where
    first (Pair x y) = x
    second (Pair x y) = y

instance Product (,) where
    first (x, y) = x
    second (x, y) = y


-- forall morphisms f1 : X -> P1, f2 : X -> P2
type X = Int
type P1 = Int
type P2 = Int
f1 :: X -> P1
f1 = (+3)
f2 :: X -> P2
f2 = (*2)

-- there exists a unique morphism f : X -> Pair P1 P2
-- such that `first . f == f1`  and  `second . f == f2`.
--
makepair :: X -> Pair P1 P2
makepair x = Pair (f1 x) (f2 x)

main = do
    print $ (first . makepair) 1
    print $ f1 1

    print $ (second . makepair) 1
    print $ f2 1
