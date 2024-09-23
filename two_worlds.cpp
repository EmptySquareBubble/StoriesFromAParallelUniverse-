#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>

int next_gen(int next_gen_pray, int current_gen, bool depends_on_food)
{
    //do some expensive magic to enforce parallelization
    auto dummy_expensive_value = current_gen;
    for(int i = 0; i < current_gen * 10; ++i)
        dummy_expensive_value += std::sqrt(dummy_expensive_value);
    
    //how many predators may survive with given amount of pray
    const auto portions = next_gen_pray / 4;
    return depends_on_food ? std::max(current_gen, portions) : (current_gen);
}

void next_year_population(bool depends_on_food)
{
    std::vector<int> food_chain{40, 4, 25, 5, 128, 23, 1029, 43567, 23, 56, 134};

    auto next_gen_seq = std::accumulate(food_chain.begin() + 1, food_chain.end(), food_chain[0], [depends_on_food](const auto first, const auto second){
        return first + next_gen(first, second, depends_on_food);});
    auto next_gen_par = std::reduce(std::execution::par, food_chain.begin() + 1, food_chain.end(), food_chain[0], [depends_on_food](const auto first, const auto second){
        return first + next_gen(first, second, depends_on_food);});

    std::cout << "Total population size - " << (depends_on_food ? "depends on food available" : "no dependency") << " seq: " << next_gen_seq << " par: " << next_gen_par << std::endl;
}

const auto list_of_species = std::ranges::iota_view{0, 1700};

void build_sky_empire()
{
    std::vector<std::jthread> creatures;

    //creates thread for everything
    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto id){
        creatures.push_back(std::move(std::jthread([id]{
            std::cout << "my name is #" << id << " and I'm flying!" << std::endl;
        })));
    });

}

void build_underwater_kingdom()
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto id){
        std::cout << "my name is #" << id << " and I'm swimming!" << std::endl;
    });
}

int main()
{
    //const auto start = std::chrono::steady_clock::now();

    //build_sky_empire();
    //build_underwater_kingdom();

    next_year_population(true);
    next_year_population(false);

    //const auto end = std::chrono::steady_clock::now();
    //std::cout << "execution time: " << end - start << std::endl;

    return 0;
}