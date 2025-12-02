#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <string>

// タスクの優先度
enum class TaskPriority {
    High,    // 高優先度
    Normal,  // 通常
    Low      // 低優先度
};

// タスク構造体
struct Task {
    std::string name; // タスク名
    std::function<void()> func; // タスクの実体
    TaskPriority priority = TaskPriority::Normal; // タスクの優先度
    std::vector<std::shared_future<void>> dependencies; // 依存関係タスク

    bool operator<(const Task& other) const {
        return priority < other.priority; // 優先度が高いものを先に処理
    }
};

class ThreadManager {
public:
    ThreadManager() = default;
    ~ThreadManager() = default;

    // 初期化
    void Initialize()
    {
        // CPUの論理コア数に応じたスレッドプールを作成
        uint32_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0)
        {
            numThreads = 4; // 取得できなかった場合のデフォルト
        }

        uint32_t maxThreads = std::min(numThreads * 2, 64u); // 最大スレッド数（64スレッド上限）
        m_WorkerCount = maxThreads; // スレッド数を記録

        // スレッドを最大数まで作成
        for (uint32_t i = 0; i < maxThreads; ++i)
        {
            m_Workers.emplace_back([this] { WorkerThread(); });
        }
    }

    // タスクをキューに追加（依存関係あり）
    std::shared_future<void> EnqueueTask(
        const std::string& taskName,
        std::function<void()> task,
        TaskPriority priority = TaskPriority::Normal,
        std::vector<std::shared_future<void>> dependencies = {}
    )
    {
        auto promise = std::make_shared<std::promise<void>>(); // タスク完了通知用のPromise
        auto future = promise->get_future().share(); // タスク完了通知用のFuture（共有）

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex); // キューの排他制御（ロック）
            m_TaskQueue.push({
                taskName,
                [task, promise, taskName]() { // ラムダ式によりタスクを登録
                    std::string msg = "Task: " + taskName + " is executing.";
                    Log::Write(LogLevel::Info, msg); // タスク実行中のログ出力
                    task(); // タスクの実行
                    promise->set_value(); // タスク完了を通知
                }, priority, dependencies
                });
        }
        m_Condition.notify_one(); // スレッドにタスクが追加されたことを通知
        return future; // タスク完了の通知を取得
    }

    // バッチ処理を登録（複数のタスクをまとめて処理）
    std::shared_future<void> EnqueueBatchTask(
        const std::string& batchName,
        std::vector<std::function<void()>> tasks,
        TaskPriority priority = TaskPriority::Normal
    )
    {
        auto promise = std::make_shared<std::promise<void>>(); // バッチ完了通知用
        auto future = promise->get_future().share(); // タスク完了通知用のFuture（共有）

        // バッチ処理を一つのタスクとして登録
        EnqueueTask(batchName, [tasks, promise, batchName]() {
            std::string msg = "Batch Task: " + batchName + " is executing.";
            Log::Write(LogLevel::Info, msg); // バッチ実行中のログ出力
            for (auto& task : tasks)
            {
                task(); // 各タスクを順に実行
            }
            promise->set_value(); // バッチ完了通知
            }, priority);

        return future; // バッチ処理完了の通知を取得
    }

    // 全スレッドの停止
    void StopAllThreads()
    {
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex); // 排他制御（ロック）
            m_Stop = true; // スレッド停止フラグを立てる
        }
        m_Condition.notify_all(); // すべてのスレッドを起こす

        // すべてのスレッドを終了待機
        for (std::thread& worker : m_Workers)
        {
            if (worker.joinable())
            { // スレッドが実行中なら
                worker.join(); // スレッドが終了するまで待機（同期）
            }
        }
    }

    // 現在のタスク数を取得
    size_t GetTaskCount() {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        return m_TaskQueue.size();
    }

    // 現在のスレッドの数を取得
    size_t GetThreadCount() const {
        return m_WorkerCount;
    }

    // タスクキューをクリア
    void ClearTaskQueue() {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        while (!m_TaskQueue.empty()) {
            m_TaskQueue.pop();
        }
    }

    // 指定したタスクが完了するまで待機する
    void WaitForTask(std::shared_future<void> task) {
        if (task.valid()) {
            task.wait(); // タスク完了までブロック
        }
    }

    // 全てのタスクが完了するまで待機する
    void WaitForAllTasks() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                if (m_TaskQueue.empty()) {
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // CPU負荷を抑えて待機
        }
    }

    // スレッドの数を変更
    void SetThreadCount(const uint32_t& newCount) {
        StopAllThreads(); // 既存のスレッドを停止
        m_WorkerCount = newCount;

        for (uint32_t i = 0; i < newCount; ++i) {
            m_Workers.emplace_back([this] { WorkerThread(); });
        }
    }   

private:
    // ワーカースレッドのメインループ
    void WorkerThread()
    {
        while (true)
        {
            Task task; // タスクオブジェクト
            bool hasTask = false; // タスクがあるかどうかのフラグ

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex); // 排他制御（ロック）
                m_Condition.wait(lock, [this] { // タスクが追加されるか停止するまで待機
                    return m_Stop || !m_TaskQueue.empty(); // スレッド停止か、タスクキューにタスクがある
                    });

                if (m_Stop) return; // スレッド停止フラグが立ったら終了

                // 依存関係が解決済みのタスクを探す
                std::priority_queue<Task> tempQueue; // 一時的なキュー
                while (!m_TaskQueue.empty())
                {
                    Task currentTask = std::move(const_cast<Task&>(m_TaskQueue.top())); // タスク取得
                    m_TaskQueue.pop(); // キューから削除

                    if (CanExecuteTask(currentTask))
                    { // 依存関係が解決済みなら
                        task = std::move(currentTask); // タスクを確定
                        hasTask = true; // タスクありフラグをセット
                        break;
                    }
                    else
                    {
                        tempQueue.push(std::move(currentTask)); // 実行できないタスクは一時キューへ戻す
                    }
                }
                // 実行できなかったタスクを戻す
                while (!tempQueue.empty())
                {
                    Task tempTask = std::move(tempQueue.top());
                    tempQueue.pop();
                    m_TaskQueue.push(std::move(tempTask));
                }
            }

            // 取得したタスクを実行
            if (hasTask)
            {
                task.func(); // タスク実行
            }
        }
    }
    // 依存関係が解決済みかチェック
    bool CanExecuteTask(const Task& task)
    {
        for (const auto& dep : task.dependencies)
        { // 依存関係をすべてチェック
            if (dep.valid() && dep.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                return false; // 依存タスクが未完了なら実行できない
            }
        }
        return true; // すべての依存タスクが完了していれば実行可能
    }
private:
    std::vector<std::thread> m_Workers;// ワーカースレッド
    std::priority_queue<Task> m_TaskQueue; // タスクキュー
    std::mutex m_QueueMutex;// キューの排他制御
    std::condition_variable m_Condition;// キューの待機
    std::atomic<bool> m_Stop{ false }; // スレッド停止フラグ
    uint32_t m_WorkerCount = 0; // ワーカースレッド数
};
