#ifndef SWSL_TOKDISP_JSON_H
#define SWSL_TOKDISP_JSON_H

#include "swsl_astgen_new.h"
#include "swsl_tokdisp.h"

namespace swsl
{
	class TokenDispatcher_JSON : public TokenDispatcher
	{
	private:
		int m_indent;

	public:
		TokenDispatcher_JSON( void ) : m_indent(-1) {}

		void Dispatch(new_Token *tree)
		{
			if (tree != NULL) {
				++m_indent;
				if (tree->sub == NULL) {
					// dispatch this
					Dispatch(tree->next);
				} else {
					Dispatch(tree->sub);
				}
				--m_indent;
			}
		}
	};
}

#endif // SWSL_TOKDISP_JSON_H
