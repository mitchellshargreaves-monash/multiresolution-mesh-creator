#ifndef DVIDUTILS_DOWNSAMPLE_LABELS_HPP
#define DVIDUTILS_DOWNSAMPLE_LABELS_HPP

#include <algorithm>
#include <boost/container/flat_map.hpp>

#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xvectorize.hpp"
#include "xtensor/xeval.hpp"

namespace dvidutils {
    
    template <typename label_array_t, int N>
    struct downsample_labels_functor
    {
        using result_type = xt::xarray<typename label_array_t::value_type>;
        result_type operator()( label_array_t const & labels, int factor, bool suppress_zero=false );
    };

    template <class pair_t>
    bool compare_pairs(pair_t const & p1, pair_t const & p2)
    {
        return p1.second < p2.second;
    }
    
    template <class pair_t>
    bool compare_pairs_suppress_zero(pair_t const & p1, pair_t const & p2)
    {
        if (p1.first == 0 && p2.first == 0)
        {
            return false;
        }
        if (p1.first == 0)
        {
            return true;
        }
        if (p2.first == 0)
        {
            return false;
        }
        return p1.second < p2.second;
    }
    
    
    template <typename label_array_t>
    struct downsample_labels_functor<label_array_t, 3>
    {
        using result_type = xt::xarray<typename label_array_t::value_type>;
        result_type operator()( label_array_t const & labels, int factor, bool suppress_zero=false )
        {
            using label_t = typename label_array_t::value_type;
            
            std::vector<int> output_shape = {};
            for (auto s : labels.shape())
            {
                output_shape.push_back(s);
            }
            for (auto & s : output_shape)
            {
                if (s == 0) {
                    // Technically, we could omit this check -- it should simply result in a zero-size output.
                    std::ostringstream ss;
                    ss << "Precondition violation: zero-size array.  Shape: (";
                    for (auto d : labels.shape())
                    {
                        ss << d << ", ";
                    }
                    ss << ")";
                    throw std::runtime_error(ss.str().c_str());
                }
                if (s % factor != 0)
                {
                    std::ostringstream ss;
                    ss << "Precondition violation: Downsampling factor must divide cleanly into array shape: (";
                    for (auto d : labels.shape())
                    {
                        ss << d << ", ";
                    }
                    ss << ")";
                    throw std::runtime_error(ss.str().c_str());
                }
                
                s /= factor;
            }

            auto res = result_type::from_shape(output_shape);

            // Create just one flat_map to re-use for every block
            boost::container::flat_map<label_t, int> counts;
            
            for (size_t z_res = 0; z_res < output_shape[0]; ++z_res)
            {
                size_t z = z_res*factor;
                for (size_t y_res = 0; y_res < output_shape[1]; ++y_res)
                {
                    size_t y = y_res*factor;
                    for (size_t x_res = 0; x_res < output_shape[2]; ++x_res)
                    {
                        size_t x = x_res*factor;
                        
                        auto block = xt::view(labels, xt::range(z,z+factor), xt::range(y,y+factor), xt::range(x,x+factor));
                        
                        // Load the counts
                        counts.clear();
                        xt::eval(xt::vectorize([&](label_t label) { counts[label] += 1; return 0;})(block));
                        
                        // Find the maximum count
                        // Note: This function guarantees that ties are resolved in favor of the lower value.
                        //       Since flat_map is ordered, and max_element chooses the first tied value,
                        //       we're in compliance with that guarantee.
                        auto max_pair = counts.begin();
                        if (suppress_zero)
                        {
                            max_pair = std::max_element( counts.begin(), counts.end(),
                                                        compare_pairs_suppress_zero<typename decltype(counts)::value_type>);
                        }
                        else
                        {
                            max_pair = std::max_element( counts.begin(), counts.end(),
                                                        compare_pairs<typename decltype(counts)::value_type>);
                        }
                        
                        res(z_res, y_res, x_res) = max_pair->first;
                    }
                }
            }
            
            return res;
        }
    };

    template <typename label_array_t>
    struct downsample_labels_functor<label_array_t, 2>
    {
        using result_type = xt::xarray<typename label_array_t::value_type>;
        result_type operator()( label_array_t const & labels, int factor, bool suppress_zero=false )
        {
            using label_t = typename label_array_t::value_type;

            // FIXME: Why is this weird shape initialization necessary?
            //        When I tried something more straightforward, (auto output_shape = labels.shape())
            //        It didn't give me a copy, it gave me some weird adaptor that was a reference to the original...
            std::vector<int> output_shape = {};
            for (auto s : labels.shape())
            {
                output_shape.push_back(s);
            }
            for (auto & s : output_shape)
            {
                if (s == 0) {
                    // Technically, we could omit this check -- it should simply result in a zero-size output.
                    std::ostringstream ss;
                    ss << "Precondition violation: zero-size array.  Shape: (";
                    for (auto d : labels.shape())
                    {
                        ss << d << ", ";
                    }
                    ss << ")";
                    throw std::runtime_error(ss.str().c_str());
                }
                if (s % factor != 0)
                {
                    std::ostringstream ss;
                    ss << "Precondition violation: Downsampling factor must divide cleanly into array shape: (";
                    for (auto d : labels.shape())
                    {
                        ss << d << ", ";
                    }
                    ss << ")";
                    throw std::runtime_error(ss.str().c_str());
                }
                
                s /= factor;
            }
            auto res = result_type::from_shape(output_shape);

            // Create just one flat_map to re-use for every block
            boost::container::flat_map<label_t, int> counts;
            for (size_t y_res = 0; y_res < output_shape[0]; ++y_res)
            {
                size_t y = y_res*factor;
                for (size_t x_res = 0; x_res < output_shape[1]; ++x_res)
                {
                    size_t x = x_res*factor;
                    
                    auto block = xt::view(labels, xt::range(y,y+factor), xt::range(x,x+factor));
                    
                    // Load the counts
                    counts.clear();
                    xt::eval(xt::vectorize([&](label_t label) { counts[label] += 1; return 0;})(block));
                    
                    // Find the maximum count
                    // Note: This function guarantees that ties are resolved in favor of the lower value.
                    //       Since flat_map is ordered, and max_element chooses the first tied value,
                    //       we're in compliance with that guarantee.
                    auto max_pair = counts.begin();
                    if (suppress_zero)
                    {
                        max_pair = std::max_element( counts.begin(), counts.end(),
                                                    compare_pairs_suppress_zero<typename decltype(counts)::value_type>);
                    }
                    else
                    {
                        max_pair = std::max_element( counts.begin(), counts.end(),
                                                    compare_pairs<typename decltype(counts)::value_type>);
                    }
                    
                    res(y_res, x_res) = max_pair->first;
                }
            }
            
            return res;
        }
    };
    
    template <typename labelarray_t, int N>
    labelarray_t downsample_labels(labelarray_t const & labels, int factor, bool suppress_zero )
    {
        return downsample_labels_functor<labelarray_t, N>()(labels, factor, suppress_zero);
    }
}

#endif
