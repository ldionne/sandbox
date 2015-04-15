import Prelude hiding (elem)

newtype S a = S {find :: (a -> Bool) -> a}

search :: S a -> (a -> Bool) -> Maybe a
search xs p = let x = find xs p in if p x then Just x else Nothing

forsome, forevery :: S a -> (a -> Bool) -> Bool
forsome xs p = p(find xs p)
forevery xs p = not(forsome xs (\x -> not(p x)))

elem :: Eq a => a -> S a -> Bool
elem x xs = forsome xs (x ==)

subset :: Eq a => S a -> S a -> Bool
subset xs ys = forevery xs (\x -> x `elem` ys)

singleton :: a -> S a
singleton x = S(\p -> x)

doubleton :: a -> a -> S a
doubleton x y = S(\p -> if p x then x else y)

image :: (a -> b) -> S a -> S b
image f xs = S(\q -> f(find xs (\x -> q(f x))))

bigUnion :: S(S a) -> S a
bigUnion xss = S(\p -> find(find xss (\xs -> forsome xs p)) p)

union :: S a -> S a -> S a
xs `union` ys = bigUnion(doubleton xs ys)

instance Eq a => Eq (S a) where
    xs == ys = (subset xs ys) && (subset ys xs)

instance Monad S where
  return = singleton
  xs >>= f = bigUnion(image f xs)

times :: S a -> S b -> S(a,b)
xs `times` ys = do x <- xs
                   y <- ys
                   return(x,y)

bit :: S Int
bit = doubleton 0 1

cantor :: S [Int]
cantor = sequence (repeat bit)


main = do
    print $ forsome cantor (\s -> s == (take 9999 $ repeat 1))
