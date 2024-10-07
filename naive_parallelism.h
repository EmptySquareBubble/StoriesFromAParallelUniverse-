#pragma once

template<typename I, typename J, typename K, typename T, typename R>
K naive_transform_reduce_body(I a_start, I a_end, J b_start, K init, T transform, R reduce)
{
    if(std::distance(a_start, a_end) == 1)
        return transform(*a_start, *b_start);

    const auto middle = std::distance(a_start, a_end) / 2;
    const auto left = naive_transform_reduce_body(a_start, a_start + middle, b_start, init, transform, reduce);
    const auto right = naive_transform_reduce_body(a_start + middle, a_end, b_start + middle, init, transform, reduce);

    return reduce(left, right);
}

template<typename I, typename J, typename K, typename T, typename R>
K naive_transform_reduce(I a_start, I a_end, J b_start, K init, T transform, R reduce)
{
    const K body = naive_transform_reduce_body(a_start, a_end, b_start, init, transform, reduce);
    return reduce(init, body);
}

template<typename I, typename K, typename T, typename R>
K naive_transform_reduce(I a_start, I a_end, K init, T transform, R reduce)
{
    auto transform_wrapper = [&](const auto i, const auto j){return transform(i);};
    auto body = naive_transform_reduce_body(a_start, a_end, a_start, init, transform_wrapper, reduce);
    return reduce(init, body);
}

template<typename I, typename K, typename R>
K naive_reduce(I a_start, I a_end, K init, R reduce)
{
    return naive_transform_reduce(a_start, a_end, a_start, init, [](const auto i, const auto j){return i;}, reduce);
}

template<typename I, typename T>
void naive_inclusive_scan(I input, I& output, T transform)
{
    size_t idx = 1;
    while(idx < output.size())
    {
        for(size_t i = 0; i < idx; ++i)
            output[i] = input[i];
        for(size_t i = idx; i < output.size(); ++i)
            output[i] = transform(input[i - idx], input[i]);
        idx = 1 << idx;
        
        std::swap(input, output);
    }
    std::swap(input, output);
}