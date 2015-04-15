{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE UndecidableInstances #-}

import Prelude hiding (foldr)
import Data.Foldable

class ExoFoldable t where
    exofoldr :: ((x -> s -> s) -> s -> t x -> s) -> (x -> s -> s) -> s -> t x -> s

instance ExoFoldable t => Foldable t where
    foldr f s xs = exofoldr Data.Foldable.foldr f s xs

instance ExoFoldable [] where
    exofoldr exo f s (x:xs) = f x (exo f s xs)
    exofoldr _ _ s [] = s

instance ExoFoldable Maybe where
    exofoldr _ f s (Just x) = f x s
    exofoldr _ _ s Nothing = s


-----------------

data Tree a = Empty | Leaf a | Node (Tree a) a (Tree a)

instance ExoFoldable Tree where
    exofoldr exo f z Empty = z
    exofoldr exo f z (Leaf x) = f x z
    exofoldr exo f z (Node l k r) = exo f (f k (exo f z r)) l


extend :: [x] -> [x]
extend xs = go (:) [] xs
    where go f = exofoldr go (\x xs -> f x (x:xs))


tree = Node (Leaf 1) 2 (Leaf 3)

main = do
    print $ extend [1, 2, 3]
    print $ foldr (+) 0 tree
