#pragma once
// === C++ Standard Library ===
#include <vector>
#include <queue>
#include <string>
#include <string_view>
#include <cstdint>
#include <algorithm>

#include "include/Platform/Thread.h"

namespace Theatria
{
    using namespace Platform::Threading;
    namespace Core
    {
        class JobSystem final
        {
        public:

            enum class JobPriority : uint8_t
            {
                High,
                Normal,
                Low
            };

            /// @brief ジョブ構造体
            struct Job final
            {
                std::string name;///< ジョブ名
                std::function<void()> func;///< ジョブ関数
                JobPriority priority = JobPriority::Normal;///< ジョブ優先度
                std::vector<std::shared_future<void>> dependencies;///< 依存ジョブ

                /// @brief 優先度比較演算子
                bool operator<(const Job& other) const noexcept
                {
                    return static_cast<uint8_t>(priority) < static_cast<uint8_t>(other.priority);
                }
            };

            JobSystem() = default;
            ~JobSystem() = default;

            /// @brief 初期化
            void Initialize()
            {
                // CPUの論理コア数に応じたスレッドプールを作成
                uint32_t numWorkers = GetHardwareThreadCount();
                if (numWorkers == 0)
                {
                    numWorkers = 4;// 取得できなかった時場合は4に設定
                }

                uint32_t maxWorkers = std::min(numWorkers * 2, 64u);// 最大スレッド数を64に制限
                m_WorkerCount = maxWorkers;// ワーカースレッド数設定

                for ([[maybe_unused]] uint32_t i = 0; i < m_WorkerCount; i++)
                {
                    m_Workers.emplace_back([this](StopToken st) {WorkerThread(st); });
                }
            }

            /// @brief ジョブ追加
            std::shared_future<void> EnqueueJob(
                std::string jobName,
                std::function<void()> job,
                JobPriority priority = JobPriority::Normal,
                std::vector<std::shared_future<void>> dependencies = {})
            {
                auto promise = std::make_shared<std::promise<void>>(); // タスク完了通知用のPromise
                auto future = promise->get_future().share(); // タスク完了通知用のFuture（共有）

                {
                    std::lock_guard<std::mutex> lock(m_QueueMutex); // キューの排他制御（ロック）
                    m_JobQueue.push({
                        jobName,
                        [job, promise, jobName]() { // ラムダ式によりタスクを登録
                            // std::string msg = "Task: " + taskName + " is executing.";
                            // Log::Write(LogLevel::Info, msg); // タスク実行中のログ出力
                            jobName;
                            job(); // タスクの実行
                            promise->set_value(); // タスク完了を通知
                        }, priority, dependencies
                        });
                }
                m_Cv.notify_one(); // スレッドにタスクが追加されたことを通知
                return future; // タスク完了の通知を取得
            }

            /// @brief バッチ処理を登録（複数のタスクをまとめて処理）
            std::shared_future<void> EnqueueBatchJob(
                const std::string& batchName,
                std::vector<std::function<void()>> jobs,
                JobPriority priority = JobPriority::Normal
            )
            {
                auto promise = std::make_shared<std::promise<void>>(); // バッチ完了通知用
                auto future = promise->get_future().share(); // タスク完了通知用のFuture（共有）

                // バッチ処理を一つのタスクとして登録
                EnqueueJob(batchName, [jobs, promise, batchName]() {
                    for (auto& job : jobs)
                    {
                        job(); // 各タスクを順に実行
                    }
                    promise->set_value(); // バッチ完了通知
                    }, priority);

                return future; // バッチ処理完了の通知を取得
            }

            /// @brief 全スレッド停止
            void StopAllThreads()
            {
                {
                    std::lock_guard<std::mutex> lock(m_QueueMutex); // 排他制御（ロック）
                    m_Stop = true; // スレッド停止フラグを立てる
                }
                m_Cv.notify_all(); // すべてのスレッドを起こす

                // すべてのスレッドを終了待機
                for (Thread& worker : m_Workers)
                {
                    if (worker.Joinable())
                    { // スレッドが実行中なら
                        worker.Join(); // スレッドが終了するまで待機（同期）
                    }
                }
            }

            // 現在のタスク数を取得
            size_t GetJobCount()
            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                return m_JobQueue.size();
            }

            // 現在のスレッドの数を取得
            size_t GetThreadCount() const
            {
                return m_WorkerCount;
            }

            // タスクキューをクリア
            void ClearJobQueue()
            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                while (!m_JobQueue.empty())
                {
                    m_JobQueue.pop();
                }
            }

            // 指定したタスクが完了するまで待機する
            void WaitForJob(std::shared_future<void> job)
            {
                if (job.valid())
                {
                    job.wait(); // タスク完了までブロック
                }
            }

            // 全てのタスクが完了するまで待機する
            void WaitForAllJobs()
            {
                while (true)
                {
                    {
                        std::lock_guard<std::mutex> lock(m_QueueMutex);
                        if (m_JobQueue.empty())
                        {
                            return;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // CPU負荷を抑えて待機
                }
            }

            // スレッドの数を変更
            void SetThreadCount(const uint32_t& newCount)
            {
                StopAllThreads(); // 既存のスレッドを停止
                m_WorkerCount = newCount;

                for (uint32_t i = 0; i < newCount; ++i)
                {
                    m_Workers.emplace_back([this](StopToken st) { WorkerThread(st); });
                }
            }

        private:
            /// @brief ワーカースレッド関数
            /// @param st 
            void WorkerThread(StopToken st)
            {
                while (!st.stop_requested())
                {
                    Job job;///< ジョブオブジェクト
                    bool hasJob = false;
                    hasJob;
                    {
                        UniqueLock lock(m_QueueMutex);// キューのロック
                        m_Cv.wait(lock, [this]() {
                            return m_Stop.load() || !m_JobQueue.empty();// スレッド停止要求、ジョブキューにジョブがある時にtrue
                            });

                        if (m_Stop.load()) { return; } // スレッド停止要求

                        // 依存関係が解決済みのジョブを探す
                        std::priority_queue<Job> tempQueue;// 一時キュー
                        while (!m_JobQueue.empty())
                        {
                            Job currentJob = std::move(const_cast<Job&>(m_JobQueue.top()));// キューの先頭ジョブを取得
                            m_JobQueue.pop();// キューから削除

                            if (CanExecuteJob(currentJob))
                            {
                                // 依存関係が解決済みのジョブを実行
                                job = std::move(currentJob);
                                hasJob = true;
                                break;
                            }
                            else
                            {
                                tempQueue.push(std::move(currentJob));// 依存関係が未解決のジョブは一時キューに移動
                            }
                        }
                        // 実行できなかったジョブを元のキューに戻す
                        while (!tempQueue.empty())
                        {
                            Job tempJob = tempQueue.top();// コピー
                            tempQueue.pop();
                            m_JobQueue.push(std::move(tempJob));
                        }
                    }

                    // ジョブ実行
                    if (hasJob)
                    {
                        job.func();// ジョブ関数の実行
                    }
                }
            }

            /// @brief 依存関係のジョブが完了しているか確認
            /// @param job 
            /// @return 
            bool CanExecuteJob(const Job& job) const noexcept
            {
                for (const auto& dep : job.dependencies)
                {
                    if (dep.valid() && dep.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                    {
                        return false; // 依存ジョブが未完了
                    }
                }
                return true; // すべての依存ジョブが完了
            }
        private:
            std::vector<Thread> m_Workers;
            std::priority_queue<Job> m_JobQueue;
            Mutex m_QueueMutex;
            ConditionVariable m_Cv;
            AtomicBool m_Stop = false;
            uint32_t m_WorkerCount = 0;
        };
    }
} // namespace Theatria::Core

