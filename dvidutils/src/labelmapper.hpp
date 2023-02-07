#ifndef DVIDUTILS_LABELMAPPER_HPP
#define DVIDUTILS_LABELMAPPER_HPP

#include <utility>
#include <unordered_map>

#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xnoalias.hpp"
#include "xtensor/xvectorize.hpp"

namespace dvidutils
{

    // Stores a mapping from an original set of labels (the domain)
    // to a new set of labels (the codomain), and exposes a function "apply()"
    // to convert arrays of domain label voxels into arrays of codomain label voxels.
    template<typename domain_t, typename codomain_t>
    class LabelMapper
    {
    public:
        typedef std::unordered_map<domain_t, codomain_t> mapping_t;

        typedef xt::xarray<domain_t> domain_array_t;
        typedef xt::xarray<codomain_t> codomain_array_t;

        class KeyError : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };
        
        // Construct directly from a pre-existing mapping
        LabelMapper(mapping_t mapping)
        : _mapping(std::move(mapping))
        {
        }

        // Construct from domain and codomain lists
        template <typename domain_list_t, typename codomain_list_t>
        LabelMapper(domain_list_t const & domain, codomain_list_t const & codomain)
        {
            if (domain.shape().size() != 1 || codomain.shape().size() != 1)
            {
                throw std::runtime_error("Can't initialize LabelMapper: domain and codomain should be 1D arrays");
            }
            if (domain.shape()[0] != codomain.shape()[0])
            {
                throw std::runtime_error("Can't initialize LabelMapper: "
                                         "domain and codomain arrays don't have matching sizes.");
            }

            // Load up the mapping
            for (size_t i = 0; i < domain.shape()[0]; ++i)
            {
                _mapping[domain(i)] = codomain(i);
            }
        }

        template <typename array_t>
        codomain_array_t apply( array_t const & src, bool allow_unmapped=false )
        {
            auto res = codomain_array_t::from_shape(src.shape());
            _apply_impl(src, res, allow_unmapped, 0, false);
            return res;
        }

        template <typename array_t>
        codomain_array_t apply_with_default( array_t const & src, typename array_t::value_type default_value=0 )
        {
            auto res = codomain_array_t::from_shape(src.shape());
            _apply_impl(src, res, true, default_value, true);
            return res;
        }
        // FIXME: It would be nice to figure out how to allow unified function
        //        signatures that handle in-place and non-in-place calls...
        template <typename array_t>
        void apply_inplace( array_t & src, bool allow_unmapped=false )
        {
            _apply_impl(src, src, allow_unmapped, 0, false);
        }

    private:
        
        template <typename input_array_t, typename output_array_t>
        void _apply_impl( input_array_t const & src, output_array_t & res, bool allow_unmapped,
                         typename output_array_t::value_type default_value, bool use_default )
        {
            typedef typename input_array_t::value_type input_dtype;
            typedef typename output_array_t::value_type output_dtype;
            
            // We assume the global mapping may be quite large,
            // but each input array apply() probably contains duplicate values.
            // Caching the mapping values found in src gives a ~10x speed boost.
            // The cached mapping type is based on the INPUT array dtypes (not the stored mapping dtypes)
            
            // This cached mapping is stored in terms of the input/output arrays,
            // because it will also store 'identity' entries.
            typedef std::unordered_map<input_dtype, output_dtype> cached_mapping_t;

            cached_mapping_t cached_mapping;
            
            auto lookup_voxel = [&](input_dtype px) -> output_dtype {
                
                auto cache_iter = cached_mapping.find(px);
                if (cache_iter != cached_mapping.end())
                {
                    return cache_iter->second;
                }
                
                auto iter = _mapping.find(px);
                if (iter != _mapping.end())
                {
                    auto value = iter->second;
                    cached_mapping[px] = value;
                    return value;
                }
                
                if (allow_unmapped)
                {
                    // Key is missing.
                    // Return the original value or the default value, depending on use_default.
                    auto value = default_value;
                    if (!use_default)
                    {
                        value = static_cast<output_dtype>(px);
                    }
                    cached_mapping[px] = value;
                    return value;
                }
                
                throw KeyError("Label not found in mapping: " + std::to_string(+px));
                return 0; // unreachable line
            };
            
            xt::noalias(res) = xt::vectorize(lookup_voxel)(src);
        }
        
    private:
        mapping_t _mapping;
    };
}

#endif
