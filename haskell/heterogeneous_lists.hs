{-# LANGUAGE ExistentialQuantification #-}

data T = forall a. MkT a



main = return 0
