#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>
#include <numeric>

constexpr int portion_size = 1;
constexpr int apetit_multiplier = 1;

void dummy_expensive_computation(int seed)
{
    double out = seed;
    for(int i = 0; i < seed * 1000; ++i)
        out += std::sqrt(i * std::rand());
}

void portions_per_species(const std::vector<int>& species_population)
{
    int weight_multiplier = 2;
    //exclusive scan from partial_sum
    //serial solution
    std::vector<int> portions_sum(species_population.size());
    std::partial_sum(species_population.begin(), species_population.end(), portions_sum.begin(), [&weight_multiplier](const auto& prays_portions, const auto& prays_count){
        const auto out = prays_portions + prays_count * weight_multiplier;
        weight_multiplier *= 2;
        return out;
        });
    
    std::cout << "portions avalilable for each species (partial_sum): ";
    for(const auto& portion : portions_sum)
        std::cout << portion << ",";
    std::cout << "\n";

    //parallelizable solution
    weight_multiplier = 2;
    std::vector<int> portions_scanned(species_population.size());
    std::inclusive_scan(std::execution::par, species_population.begin(), species_population.end(), portions_scanned.begin(), [&weight_multiplier](const auto& prays_portions, const auto& prays_count){
        dummy_expensive_computation(prays_count);
        const auto out = prays_portions + prays_count * weight_multiplier;
        weight_multiplier *= 2;
        return out;
        });
    
    std::cout << "portions avalilable for each species (inclusive_scan): ";
    for(const auto& portion : portions_scanned)
        std::cout << portion << ",";
    std::cout << "\n";

    //fixed parallelizable solution
    std::vector<int> weights(species_population.size());
    int n = 1;
    std::generate(weights.begin(), weights.end(), [&n]() { auto oldN = n; n *= 2; return oldN;});
    std::vector<int> weighted_population(species_population.size());
    std::transform(species_population.begin(), species_population.end(), weights.begin(), weighted_population.begin(), [](const auto& a, const auto& b){ return a * b;} );
    std::cout << "transformed: \n";
    for(const auto portion : weighted_population)
        std::cout << portion << ", ";
    std::cout << "\n";

    std::cout << "inclusive scan: \n";
    std::vector<int> portions_trans_scanned(species_population.size());
    std::inclusive_scan(std::execution::par, weighted_population.begin(), weighted_population.end(), portions_trans_scanned.begin(), [&](const auto& prays_portions, const auto& prays_count){
        dummy_expensive_computation(prays_count);
        return prays_portions + prays_count;
        });
    
    std::cout << "portions avalilable for each species (inclusive_scan after transform): ";
    for(const auto& portion : portions_trans_scanned)
        std::cout << portion << ",";
    std::cout << "\n";

    //pipe it togehter
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