#include <fstream>
#include "swsl_json.h"

void PrintStr(mtlChars str, std::ofstream &fout)
{
	str = str.GetTrimmed();
	bool last_white = true;
	for (int i = 0; i < str.GetSize(); ++i) {
		char ch = str[i];
		bool is_white = mtlChars::IsWhitespace(ch);
		if (last_white && is_white) { continue; }
		if (is_white)               { fout << " "; }
		else                        { fout << ch; }
		last_white = is_white;
	}
}

void PrintTabs(int tabs, std::ofstream &fout)
{
	for (int i = 0; i < tabs; ++i) {
		fout << "\t";
	}
}

bool SerializeToJSON(const new_Token *tree, std::ofstream &fout, int tab)
{
	if (tree == NULL) { return true; }
	PrintTabs(tab, fout);
	fout << "{\n";

	PrintTabs(tab + 1, fout);
	fout << "\"type\" : \"";
	switch (tree->type) {
	case new_Token::BOOL_EXPR:
		fout << "BOOL_EXPR";
		break;
	case new_Token::BOOL_OP:
		fout << "BOOL_OP";
		break;
	case new_Token::ELSE:
		fout << "ELSE";
		break;
	default:
	case new_Token::ERR:
		fout << "ERR";
		break;
	case new_Token::EXPR:
		fout << "EXPR";
		break;
	case new_Token::FILE:
		fout << "FILE";
		break;
	case new_Token::FLOAT_EXPR:
		fout << "FLOAT_EXPR";
		break;
	case new_Token::FLOAT_OP:
		fout << "FLOAT_OP";
		break;
	case new_Token::FN_DECL:
		fout << "FN_DECL";
		break;
	case new_Token::FN_DEF:
		fout << "FN_DEF";
		break;
	case new_Token::FN_OP:
		fout << "FN_OP";
		break;
	case new_Token::IF:
		fout << "IF";
		break;
	case new_Token::INT_EXPR:
		fout << "INT_EXPR";
		break;
	case new_Token::INT_OP:
		fout << "INT_OP";
		break;
	case new_Token::MATH_OP:
		fout << "MATH_OP";
		break;
	case new_Token::MEM_OP:
		fout << "MEM_OP";
		break;
	case new_Token::RET:
		fout << "RET";
		break;
	case new_Token::SET:
		fout << "SET";
		break;
	case new_Token::SCOPE:
		fout << "SCOPE";
		break;
	case new_Token::TYPE_DEF:
		fout << "TYPE_DEF";
		break;
	case new_Token::TYPE_NAME:
		fout << "TYPE_NAME";
		break;
	case new_Token::TYPE_TRAIT:
		fout << "TYPE_TRAIT";
		break;
	case new_Token::USR_NAME:
		fout << "USR_NAME";
		break;
	case new_Token::VAR_DECL:
		fout << "VAR_DECL";
		break;
	case new_Token::WHILE:
		fout << "WHILE";
		break;
	}
	fout << "\",\n";

	PrintTabs(tab + 1, fout);
	fout << "\"str\"  : \"";
	PrintStr(tree->str, fout);
	fout << "\",\n";

	PrintTabs(tab + 1, fout);
	fout << "\"sub\"  :";
	if (tree->sub != NULL) {
		fout << "\n";
		SerializeToJSON(tree->sub, fout, tab + 1);
	} else {
		fout << " null";
	}
	fout << ",\n";

	PrintTabs(tab + 1, fout);
	fout << "\"next\" :";
	if (tree->next != NULL) {
		fout << "\n";
		SerializeToJSON(tree->next, fout, tab + 1);
	} else {
		fout << " null";
	}
	fout << "\n";

	PrintTabs(tab, fout);
	fout << "}";
	if (tab == 0) { fout << "\n"; }
	return true;
}

bool SerializeToJSON(const new_Token *tree, const mtlChars &file)
{
	std::ofstream fout(file.GetChars());
	return fout.is_open() ? SerializeToJSON(tree, fout, 0) : false;
}
