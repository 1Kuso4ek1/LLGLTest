#pragma once
#include <Singleton.hpp>

#include <vector>
#include <future>

using namespace std::chrono_literals;

namespace dev
{

class Multithreading : public Singleton<Multithreading>
{
public:
    void Update();

    void AddJob(std::future<void>&& future);
    void AddJob(std::function<void()> job);

    void AddMainThreadJob(std::function<void()> job);

    size_t GetJobsNum() const;

private:
    std::vector<std::future<void>> jobs;
    std::vector<std::function<void()>> mainThread;
};

}
