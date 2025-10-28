#pragma once
// === C++ Standard Library ===
#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <any>
#include <mutex>
#include <optional>

namespace Theatria::Core
{
    namespace EventCommand
    {
        /// @brief 
        class EventSystem final
        {
        private:

        public:
            template<class E>
            using Handler = std::function<void(const E&)>;///< イベントハンドラー型

            /// @brief ハンドラーの登録
            /// @tparam E 
            /// @param h イベントハンドラー 
            template<class E>
            void Subscribe(Handler<E> h)
            {
                auto& vec = getVec<E>();
                vec.push_back(std::move(h));
            }

            template<class E>
            void Publish(const E& e)
            {
                auto& vec = getVec<E>();
                for (auto& h : vec) h(e);
            }
        private:

            template<class E>
            std::vector<Handler<E>>& getVec()
            {
                const std::type_index k{ typeid(E) };
                // キーが無ければ「空の vector<Handler<E>>」を作って格納
                auto [it, inserted] = m_Handlers.try_emplace(k, std::vector<Handler<E>>{});
                // any の中身を参照として取得（失敗しない経路）
                return *std::any_cast<std::vector<Handler<E>>>(&(it->second));
            }

            std::unordered_map<std::type_index, void*> m_Handlers;///< ハンドラーマップ
        };

        /// @brief コマンドシステム
        using ExecFn = void(*)(void* ctx, const void* data);

        struct CmdEntry
        {
            ExecFn exec{};
            std::vector<uint8_t> payload; // PODをそのまま詰める
        };

        class CommandBuffer final
        {
        public:
            template<class T>
            void Push(ExecFn fn, const T& pod)
            {
                CmdEntry e; e.exec = fn;
                e.payload.resize(sizeof(T));
                std::memcpy(e.payload.data(), &pod, sizeof(T));
                cmds_.push_back(std::move(e));
            }

            // 汎用実行：ctxはExecutor側のコンテキスト
            void ExecuteAll(void* ctx)
            {
                for (auto& c : cmds_)
                {
                    c.exec(ctx, c.payload.data());
                }
                cmds_.clear();
            }

            bool Empty() const { return cmds_.empty(); }

        private:
            std::vector<CmdEntry> cmds_;
        };

        // Core/IRouter.h
        class IRouter
        {
        public:
            IRouter(Core::EventCommand::EventSystem& e,
                Core::EventCommand::CommandBuffer& c)
                : m_EventSystem(e), m_CommandBuffer(c)
            {
            }
            virtual ~IRouter() = default;
            virtual void Flush() = 0;
        protected:
            
            template<class E, class F>
            void Subscribe(F&& f)
            {
                m_EventSystem.Subscribe<E>(std::forward<F>(f));
            }

        protected:
            Core::EventCommand::EventSystem& m_EventSystem;
            Core::EventCommand::CommandBuffer& m_CommandBuffer;
        };

        // Core/RouterHub.h
        class RouterHub final
        {
        public:
            template<class T, class...Args>
            T& Add(Args&&...args)
            {
                auto up = std::make_unique<T>(std::forward<Args>(args)...);
                auto& ref = *up; routers_.push_back(std::move(up)); return ref;
            }
            void FlushAll() { for (auto& r : routers_) r->Flush(); }
        private:
            std::vector<std::unique_ptr<IRouter>> routers_;
        };
    };// namespace EventCommand

    namespace Events {};
    namespace Commands {};
    namespace Routers {};

};// namespace Theatria::Core
