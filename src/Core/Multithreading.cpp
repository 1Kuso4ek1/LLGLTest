#include <Multithreading.hpp>

namespace dev
{

void Multithreading::Update()
{
    if(jobs.empty())
        for(size_t i = 0; i < mainThread.size(); i++)
        {
            mainThread[i]();
            mainThread.erase(mainThread.begin() + i);
        }

    for(size_t i = 0; i < jobs.size(); i++)
    {
        if(jobs[i].wait_for(0ms) == std::future_status::ready)
            jobs.erase(jobs.begin() + i);
    }
}

void Multithreading::AddJob(std::future<void>&& future)
{
    jobs.emplace_back(std::move(future));
}

void Multithreading::AddJob(std::function<void()> job)
{
    jobs.emplace_back(std::async(std::launch::async, job));
}

void Multithreading::AddMainThreadJob(std::function<void()> job)
{
    mainThread.push_back(job);
}

size_t Multithreading::GetJobsNum() const
{
    return jobs.size();
}

}
