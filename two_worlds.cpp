#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>

constexpr int portion_size = 2;

void balance_population(const std::vector<int>& species_population, const int factor)
{
    //simple serial version
    auto next_gen_seq = std::accumulate(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    //not processed in order! wrong result
    auto next_gen_par = std::reduce(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    //fix it it with transform reduce
    auto next_gen_par_fixed = std::transform_reduce(species_population.begin(), species_population.end(), 0, std::plus{}, [factor](const auto item){
        return item * factor;});

    //little bit more sophisticated calculation of next gen
    auto food_included = std::transform_reduce(std::execution::par, species_population.begin()+1, species_population.end(), species_population.begin(), species_population[0] * factor, std::plus{}, [factor](const auto& hunter, const auto& pray){
        const auto portions_available = pray / portion_size; 
        return std::clamp(portions_available, hunter, hunter * factor);});

    //check the results
    std::cout << "Population multiplied by factor " << factor << ":\n";
    std::cout << " seq: " << next_gen_seq << " reduce: " << next_gen_par << " transform_reduce: " << next_gen_par_fixed << std::endl;
    std::cout << "available food influences growth: " << food_included << std::endl;
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
    //build_sky_empire();
    //build_underwater_kingdom();

    std::vector<int> species_chain{68, 15, 4, 45, 18, 3};
    balance_population(species_chain, 2);

    return 0;
}