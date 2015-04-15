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
class Iterable it where
    head' :: it a -> a
    tail' :: it a -> it a
    is_empty' :: it a -> Bool

    zoom' :: Int -> it a -> (a, it a)
    zoom' 0 xs = (head' xs, tail' xs)
    zoom' n xs = zoom' (n-1) (tail' xs)


{-
-- Let It1 and It2 be Iterables. Then, an Iterable transformation f is a mapping
f' :: (Iterable it1, Iterable it2) => it1 a -> it2 a
f' = undefined

-- along with another mapping
mu :: a -> b
mu = undefined

-- such that
head' xs == head' (f xs)
tail' (f xs) == f (tail' xs)
is_empty' xs <=> is_empty (f xs)
-}

instance Iterable [] where
    head' = head
    tail' = tail
    is_empty' = null


linearize :: (Foldable f) => f x -> [x]
linearize xs = foldr (:) [] xs


main = do
    print $ (Data.Map.singleton 1 2) `Data.Map.union` (Data.Map.singleton 2 3)
    print $ (Data.Map.singleton 2 3) `Data.Map.union` (Data.Map.singleton 1 2)

    print $ zoom' 0 [0, 1, 2, 3]
    print $ zoom' 1 [0, 1, 2, 3]
    print $ zoom' 2 [0, 1, 2, 3]
    print $ zoom' 3 [0, 1, 2, 3]
