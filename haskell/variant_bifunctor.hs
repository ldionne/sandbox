-- is Variant a BiFunctor?

data Variant t u = Variant_T t | Variant_U u
data LazyVariant t u = LazyVariant_T (() -> t) | LazyVariant_U (() -> u)

class Bifunctor p where
  bimap :: (a -> b) -> (c -> d) -> p a c -> p b d


instance Bifunctor Variant where
    bimap f _ (Variant_T t) = Variant_T $ f t
    bimap _ g (Variant_U u) = Variant_U $ g u

instance Bifunctor LazyVariant where
    bimap f _ (LazyVariant_T lazy_t) = LazyVariant_T $ lazy_ft
                                        where lazy_ft () = f (lazy_t ())
    bimap _ g (LazyVariant_U lazy_u) = LazyVariant_U $ lazy_fu
                                        where lazy_fu () = g (lazy_u ())
