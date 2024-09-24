#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>
#include <numeric>

constexpr int portion_size = 1;
constexpr int apetit_multiplier = 1;

void portions_per_species(const std::vector<int>& species_population)
{
    //exclusive scan from partial_sum
    //serial solution
    std::vector<int> portions_sum = {0};
    std::partial_sum(species_population.begin(), species_population.end(), std::back_inserter(portions_sum), [&portions_sum](const auto& prays_portions, const auto& prays_count){
        return portions_sum[portions_sum.size()-1]/apetit_multiplier + prays_count/portion_size;
        });
    
    std::cout << "portions avalilable for each species (partial_sum): ";
    for(const auto& portion : portions_sum)
        std::cout << portion << ",";
    std::cout << "\n";

    //parallelizable solution
    std::vector<int> portions_scanned;
    std::exclusive_scan(species_population.begin(), species_population.end(), std::back_inserter(portions_scanned), 0, [](const auto& prays_portions, const auto& prays_count){
        return prays_portions/apetit_multiplier + prays_count/portion_size;});
    
    std::cout << "portions avalilable for each species (exclusive_scan): ";
    for(const auto& portion : portions_scanned)
        std::cout << portion << ",";
    std::cout << "\n";
}

int next_generation_size(const std::vector<int>& species_population, const int factor)
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
    //every spiecies wants to double its population, but also wants enough food for everyone 
    auto food_included = std::transform_reduce(std::execution::par, species_population.begin()+1, species_population.end(), species_population.begin(), species_population[0] * factor, std::plus{}, [factor](const auto& hunter, const auto& pray){
        const auto portions_available = pray / portion_size; 
        return std::clamp(portions_available, hunter, hunter * factor);});

    //check the results
    std::cout << "Population multiplied by factor " << factor << ":\n";
    std::cout << " seq: " << next_gen_seq << " reduce: " << next_gen_par << " transform_reduce: " << next_gen_par_fixed << std::endl;
    std::cout << "available food influences growth: " << food_included << std::endl;

    return next_gen_par_fixed;
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
    const auto creatures_cnt = next_generation_size(species_chain, 2);

    portions_per_species(species_chain);

    return 0;
}