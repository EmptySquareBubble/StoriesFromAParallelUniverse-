#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>

const auto list_of_species = std::ranges::iota_view{0, 170000};

void establish_sky_empire()
{
    std::vector<std::jthread> creatures;

    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto name){
        creatures.push_back(std::move(std::jthread([name]{
            for(auto iter = 0; iter < 5; ++iter)
            {
                std::cout << "my name is #" << name << " and I'm flying!" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds{1});
            }
        })));
    });
}

void establish_underwater_kingdom()
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto name){
        for(auto iter = 0; iter < 5; ++iter)
        {
            std::cout << "my name is #" << name << " and I'm swimming!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    });
}

int main()
{
    //establish_sky_empire();
    establish_underwater_kingdom();
    return 0;
}