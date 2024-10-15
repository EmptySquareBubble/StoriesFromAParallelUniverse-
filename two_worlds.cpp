#include <algorithm>
#include <thread>
#include <execution>
#include <print>
#include <ranges>

#include "naive_parallelism.h"

void edible_mass_avoid_sync(const std::vector<int>& species_population, const std::vector<int>& species_weight, int growth_factor)
{
    std::vector<int> portions_scanned(species_population.size());

    std::vector<int> grown_population(species_population.size());
    std::vector<int> grown_population_weight(species_population.size());
    std::transform(species_population.begin(), species_population.end(), grown_population.begin(), [&](const auto item){return item * growth_factor;});
    std::transform(grown_population.begin(), grown_population.end(), species_weight.begin(), grown_population_weight.begin(), std::multiplies{});
    std::exclusive_scan(std::execution::par_unseq, grown_population_weight.begin(), grown_population_weight.end(), portions_scanned.begin(), 0);

    std::print("\nFood amount  avalilable for each species piping\n synced between each step: ");
    for(const auto& portion : portions_scanned)
        std::print("{}g, ", portion);
    
    //pipe it nicely together - evaluation of zip_transform is on demand. no computation will be executed before item is really used
    auto weighs = std::views::zip_transform(std::multiplies{}, species_population, species_weight) | std::views::transform([&](const auto item){return item * growth_factor;});
    std::exclusive_scan(std::execution::par_unseq, weighs.begin(), weighs.end(), portions_scanned.begin(), 0);
    
    std::print("\n piped into exclusive_scan: ");
    for(const auto& portion : portions_scanned)
        std::print("{}g, ", portion);
    std::print("\n");
}

void edible_mass_no_canibalism(const std::vector<int>& species_mass, const int growth_factor)
{
    //transform before reduction to avoid associativity problem
    std::vector<int> weight_per_species;
    std::vector<int> edible_mass(species_mass.size());
    std::ranges::transform(species_mass, std::back_inserter(weight_per_species), [&](const auto item){return item * growth_factor;});
    
    std::inclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin());

    std::print("\nFood amount avalilable for each species fixed:\n canibalism allowed (inclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);

    //exclusive scan takes additional parameter (initial value) which will be assigned to very first element
    std::exclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin(), 0);

    std::print("\n canibalism not allowed (exclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);
    std::print("\n");
}

//count how much food is there available for each kind
//everyone can eat their kind and smaller
void edible_mass_per_species(const std::vector<int>& species_mass, const int growth_factor)
{
    std::vector<int> edible_mass(species_mass.size());

    int current_species_idx = 1;

    //serial solution
    std::partial_sum(species_mass.begin(), species_mass.end(), edible_mass.begin(), 
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });
    
    std::print("\nFood weight avalilable for each species next season:\n partial_sum: ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);
    
    //parallelizable solution
    std::inclusive_scan(std::execution::par, species_mass.begin(), species_mass.end(), edible_mass.begin(),
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("\n inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);

    //naive parallelizable solution
    naive_inclusive_scan(species_mass, edible_mass,
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("g\n naive inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);
    std::print("g\n");
}

void total_mass(const std::vector<int>& species_population, const std::vector<float>& species_weight)
{
    std::vector<float> float_population;
    std::transform(species_population.begin(), species_population.end(), std::back_inserter(float_population), [](int x) { return static_cast<float>(x);});

    std::print("\nWhat is total weight of whole population: \n");
    std::print(" inner_product: {}g\n", std::inner_product(float_population.begin(), float_population.end(), species_weight.begin(), 0.f));
    std::print(" transform_reduce: {}g\n", std::transform_reduce(std::execution::par_unseq, float_population.begin(), float_population.end(), species_weight.begin(), 0.f));
    std::print(" naive parallelization: {}g\n", naive_transform_reduce(float_population.begin(), float_population.end(), species_weight.begin(), 0.f, std::multiplies{}, std::plus{}));
}

void next_generation_size(const std::vector<int>& species_population, const int factor)
{
    //simple serial version
    auto next_gen_seq = std::accumulate(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    
    //not processed in order! wrong result
    auto next_gen_par = std::reduce(std::execution::par, species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    auto next_gen_par_naive = naive_reduce(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    
    //fix it it with transform reduce
    auto next_gen_par_fixed = std::transform_reduce(std::execution::par, species_population.begin(), species_population.end(), 0, std::plus{}, [factor](const auto item){
        return item * factor;});
    auto next_gen_par_naive_fixed = naive_transform_reduce(species_population.begin(), species_population.end(), 0, [factor](const auto item){
        return item * factor;}, std::plus{});
    
    std::print("\nPopulation size multiplied by factor {}:\n", factor);
    std::print(" seq: {} residents\n reduce: {} residents\n naive reduce: {} residents\n transform_reduce: {} residents\n naive transform_reduce {} residents\n", next_gen_seq, next_gen_par, next_gen_par_naive, next_gen_par_fixed, next_gen_par_naive_fixed);
}

const auto list_of_species = std::ranges::iota_view{0, 1700};

void build_sky_empire()
{
    std::vector<std::jthread> creatures;

    //creates thread for everything
    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto id){
        creatures.push_back(std::move(std::jthread([id]{
            std::print("my name is #{} and I'm flying!\n", id);
        })));
    });

}

void build_underwater_kingdom()
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto id){
        std::print("my name is #{} and I'm swimming!\n", id);
    });
}

int main()
{
    //two different world made using two different approach - which one is better and why
    //build_sky_empire();
    //build_underwater_kingdom();

    //our world has nice natural chain - here is listed size of population of each of the species ordered from the smallest/weakest prays to the biggest predators
    std::vector<int> species_chain{68, 15, 4, 45, 18, 3, 2, 11};

    //how will the total population size change in the next generation if we will expect growth by given factor
    constexpr int factor = 2;
    next_generation_size(species_chain, factor);

    //make up list of weights for each kind. increasing row as probably larger are predators and smaller prays
    const std::vector<float> species_weight{1e-6f, 2.1e-5f, 3.f, 43.3f, 5.1e3f, 6.5e4f, 7.7e6f, 1.5e8f};
    total_mass(species_chain, species_weight);
    
    //find out food options per species
    std::vector<int> species_mass;
    std::ranges::transform(std::ranges::iota_view{1, 9}, species_chain, std::back_inserter(species_mass), std::multiplies{});
    edible_mass_per_species(species_mass, factor);

    edible_mass_no_canibalism(species_mass, factor);
    
    edible_mass_avoid_sync(species_chain, std::ranges::iota_view{1, 9} | std::ranges::to<std::vector>(), factor);

    return 0;
}