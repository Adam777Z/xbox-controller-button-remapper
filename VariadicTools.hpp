#pragma once

//#include <typeinfo>

namespace handy
{
	namespace meta
	{
		template <class T1, class... T>
		struct VariadicFirst
		{
			typedef T1 type;
		};

		template <class T1, class... T>
		struct VariadicLast
		{
			typedef typename VariadicLast<T...>::type type;
		};

		template <class T1>
		struct VariadicLast<T1>
		{
			typedef T1 type;
		};
//
//		template <class T1, class... TRest>
//		struct AllButFirst
//		{
//			typedef... TRest argh;
//
//		};

		template <class... T>
		struct VariadicTemplateInfo
		{
			typedef typename VariadicFirst<T...>::type firstType;
			typedef typename VariadicLast<T...>::type  lastType;
		};

		#define VAR_IS_NOT_USED __attribute__((unused))

		template <typename T1, typename... moreT >
		T1 GetFirstVariadicValue(T1& first, moreT&... more VAR_IS_NOT_USED)
		{
			return first;
		}

	}
}


