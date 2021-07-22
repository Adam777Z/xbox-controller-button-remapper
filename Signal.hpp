#pragma once

#include <functional>
#include <map>
#include "NoCopy.hpp"
#include "VariadicTools.hpp"

namespace handy
{
	namespace arch
	{
		using SignalConnectionId_t = int;

		struct ISignalBase
		{
			virtual void disconnect(SignalConnectionId_t connection) = 0;
		};

		template <typename Ret_t, typename... Args_t>
		//template <typename Ret_t, typename FirstArg_t, typename... MoreArgs_t>
		class Signal; // Not used, but must exist to allow the specialised versions below.

		struct SignalScopedConnection : public util::NoCopy
		{
			public:
				SignalScopedConnection(ISignalBase& signal, SignalConnectionId_t id)
				: signal(signal), id(id), doDisconnection(true) {}

				SignalScopedConnection(SignalScopedConnection&& from)
				: signal(from.signal), id(from.id), doDisconnection(true) {from.doDisconnection = false;}

				~SignalScopedConnection()
				{
					// After being moved, the object being moved from's destructor will still
					// be called and, without this flag, would still disconnect the signal.
					if (doDisconnection)
						signal.disconnect(id);
				}

			private:
				ISignalBase&         signal;
				SignalConnectionId_t id;
				bool                 doDisconnection;

		};

		// Single param version
		template <typename Ret_t>
		class Signal <Ret_t ()> : public handy::util::NoCopy, public ISignalBase
		{
			public:

				using Callee_t = std::function<Ret_t()>;

				Signal()
				{
					index = 0;
				}

				// Connect, and return the connection ID. You are responsible
				// for disconnecting later using said ID, before the callback
				// goes out of scope. Otherwise said callback shall still run
				// which, seeing as it no longer exists, shall likely end poorly.
				SignalConnectionId_t connect(Callee_t callback)
				{
					auto id = makeConnId();
					callees[id] = callback;
					return id;
				}

				// Make the connection and return a SignalScopedConnection.
				// Said object will, upon its destruction, effect the disconnection of
				// the connection automatically. This means that calling this function
				// and not then assigning the result shall have no useful effect.

				//TODO: Force assignment of return value??
				SignalScopedConnection connectScoped(Callee_t callback)
				{
					auto id = makeConnId();
					callees[id] = callback;

					//SignalScopedConnection sig(*this, id);
					return std::move(SignalScopedConnection(*this, id));
				}

				// Remove the callback with the specified ID, if it exists.
				void disconnect(SignalConnectionId_t connectionId)
				{
					callees.erase(connectionId);
				}

				void call()
				{
					// Call all callbacks, provided they have a valid
					// target (checked with operator bool() in std::function)
					for (auto& callee : callees)
						if (callee.second)
							callee.second();
				}

			private:
				SignalConnectionId_t makeConnId()
				{
					index++;
					return index;
				}

				int index;
				std::map<SignalConnectionId_t, Callee_t> callees;
		};

		// Yes the no param and n>=1 param versions are completely seperate I'm afraid
		template <typename Ret_t, typename... Args_t>
		class Signal <Ret_t (Args_t...)> : public handy::util::NoCopy, public ISignalBase
		{
			public:

				using Callee_t   = std::function<Ret_t(Args_t...)>;
				using FirstArg_t = typename meta::VariadicTemplateInfo<Args_t...>::firstType;

				Signal()
				{
					index = 0;
				}

				// Connect, and return the connection ID. You are responsible
				// for disconnecting later using said ID, before the callback
				// goes out of scope. Otherwise said callback shall still run
				// which, seeing as it no longer exists, shall likely end poorly.
				SignalConnectionId_t connect(Callee_t callback)
				{
					auto id = makeConnId();
					callees[id] = callback;
					return id;
				}

				// For signals with at least one argument, request callback only if the first argument is equal to "condition"
				SignalConnectionId_t connectConditional(Callee_t callback, FirstArg_t condition)
				{
					ConditionalCallee cc = {callback, condition};
					auto id = makeConnId();
					conditionalCallees[id] = cc;
					return id;
				}

				// Make the connection and return a SignalScopedConnection.
				// Said object will, upon its destruction, effect the disconnection of
				// the connection automatically. This means that calling this function
				// and not then assigning the result shall have no useful effect.
				SignalScopedConnection connectScoped(Callee_t callback)
				{
					auto id = makeConnId();
					callees[id] = callback;

					//SignalScopedConnection sig(*this, id);
					return std::move(SignalScopedConnection(*this, id));
				}

				// Make a connection that is both conditional and scoped.
				SignalScopedConnection connectConditionalScoped(Callee_t callback, FirstArg_t condition)
				{
					ConditionalCallee cc = {callback, condition};
					auto id = makeConnId();
					conditionalCallees[id] = cc;
					return std::move(SignalScopedConnection(*this, id));
				}

				// Remove the callback with the specified ID, if it exists.
				void disconnect(SignalConnectionId_t connectionId)
				{
					callees.erase(connectionId);
				}

//				template <class FA_t, class... MA_t>
//				void call(FA_t firstArg, MA_t... moreArgs)
//				{
//					// Call all callbacks, provided they have a valid
//					// target (checked with operator bool() in std::function)
//					for (auto& callee : callees)
//						if (callee.second)
//							callee.second(firstArg, moreArgs...);
//
//					for (auto& condCallee : conditionalCallees)
//						if (condCallee.second.callee)
//							if (condCallee.second.condition == firstArg)
//								condCallee.second.callee(firstArg, moreArgs...);
//				}


				void call(Args_t... args)
				{
					// Call all callbacks, provided they have a valid
					// target (checked with operator bool() in std::function)
					for (auto& callee : callees)
						if (callee.second)
							callee.second(args...);

					auto firstArg = meta::GetFirstVariadicValue(args...);
					for (auto& condCallee : conditionalCallees)
						if (condCallee.second.condition == firstArg)
							if (condCallee.second.callee)
								condCallee.second.callee(args...);
				}


			private:
				SignalConnectionId_t makeConnId()
				{
					index++;
					return index;
				}

				int index;
				std::map<SignalConnectionId_t, Callee_t> callees;

				struct ConditionalCallee
				{
					Callee_t callee;
					typename meta::VariadicTemplateInfo<Args_t...>::firstType condition;
				};
				std::map<SignalConnectionId_t, ConditionalCallee> conditionalCallees;

		};




//		template <typename Ret_t, typename FirstArg_t, typename... MoreArgs_t>
//		class Signal <Ret_t (FirstArg_t, MoreArgs_t...)> : public handy::util::NoCopy, public ISignalBase
//		{
//			public:
//
//				using Callee_t   = std::function<Ret_t(FirstArg_t, MoreArgs_t...)>;
//				//using FirstArg_t = typename meta::VariadicTemplateInfo<Args_t...>::firstType;
//
//				Signal()
//				{
//					index = 0;
//				}
//
//				// Connect, and return the connection ID. You are responsible
//				// for disconnecting later using said ID, before the callback
//				// goes out of scope. Otherwise said callback shall still run
//				// which, seeing as it no longer exists, shall likely end poorly.
//				SignalConnectionId_t connect(Callee_t callback)
//				{
//					auto id = makeConnId();
//					callees[id] = callback;
//					return id;
//				}
//
//				// For signals with at least one argument, request callback only if the first argument is equal to "condition"
//				SignalConnectionId_t connectConditional(Callee_t callback, FirstArg_t condition)
//				{
//					ConditionalCallee cc = {callback, condition};
//					auto id = makeConnId();
//					conditionalCallees[id] = cc;
//					return id;
//				}
//
//				// Make the connection and return a SignalScopedConnection.
//				// Said object will, upon its destruction, effect the disconnection of
//				// the connection automatically. This means that calling this function
//				// and not then assigning the result shall have no useful effect.
//				SignalScopedConnection connectScoped(Callee_t callback)
//				{
//					auto id = makeConnId();
//					callees[id] = callback;
//
//					//SignalScopedConnection sig(*this, id);
//					return std::move(SignalScopedConnection(*this, id));
//				}
//
//				// Remove the callback with the specified ID, if it exists.
//				void disconnect(SignalConnectionId_t connectionId)
//				{
//					callees.erase(connectionId);
//				}
//
////				template <class FA_t, class... MA_t>
////				void call(FA_t firstArg, MA_t... moreArgs)
////				{
////					// Call all callbacks, provided they have a valid
////					// target (checked with operator bool() in std::function)
////					for (auto& callee : callees)
////						if (callee.second)
////							callee.second(firstArg, moreArgs...);
////
////					for (auto& condCallee : conditionalCallees)
////						if (condCallee.second.callee)
////							if (condCallee.second.condition == firstArg)
////								condCallee.second.callee(firstArg, moreArgs...);
////				}
//
//				SignalConnectionId_t makeConnId()
//				{
//					index++;
//					return index;
//				}
//
//			private:
//				int index;
//				std::map<SignalConnectionId_t, Callee_t> callees;
//
//				struct ConditionalCallee
//				{
//					Callee_t callee;
//					FirstArg_t condition;
//				};
//				std::map<SignalConnectionId_t, ConditionalCallee> conditionalCallees;
//
//		};




	}
}



