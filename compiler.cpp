#include "compiler.h"

#include "MiniLib/MTL/mtlStringMap.h"

class TypeList;
class ScopedTypeList;
class Type;
typedef mtlShared<Type> TypeRef;
class TypeDecl;
typedef mtlShared<TypeDecl> TypeDeclRef;

class TypeDecl
{
private:
	TypeRef      m_type;
	mtlChars     m_name;
	swsl::addr_t m_rel_addr;
};

class Type
{
public:
	enum Variant
	{
		Void,
		Bool,
		Int,
		Float,
		Composite
	};

private:
	mtlChars              m_name;
	mtlArray<TypeDeclRef> m_members;
	swsl::addr_t          m_size;
	swsl::addr_t          m_offset;
	Variant               m_variant;

public:
	bool    IsBaseType( void )              const { return m_variant != Composite; }
	bool    IsUserType( void )              const { return !IsBaseType(); }
	TypeRef GetMember(int i)                const;
	TypeRef GetMember(const mtlChars &name) const;
	int     GetMemberCount( void )          const { return IsBaseType() ? m_size : m_members.GetSize(); }

	const mtlChars &GetName( void )         const { return m_name; }
	swsl::addr_t    GetSize( void )         const { return m_size; }
	swsl::addr_t    GetMemoryOffset( void ) const { return m_offset; }
	Variant         GetVariant( void )      const { return m_variant; }

	bool Declare(const mtlChars &user_def, const ScopedTypeList &list);
};

TypeRef Type::GetMember(int i) const
{
	TypeRef ret_type;

	if (IsBaseType()) {
		const mtlChars mem_names = "xyzw";
		if (i >= 0 && i < m_size && i < mem_names.GetSize()) {
			ret_type.New();
			ret_type->m_name    = mtlChars(mem_names, i, i + 1);
			ret_type->m_size    = 1;
			ret_type->m_offset  = i;
			ret_type->m_variant = m_variant;
		}
	} else if (i >= 0 && i < m_members.GetSize()) {
		ret_type = m_members[i];
	}

	return ret_type;
}

TypeRef Type::GetMember(const mtlChars &name) const
{
	const mtlChars mem_names[4] = { mtlChars("x"), mtlChars("y"), mtlChars("z"), mtlChars("w") };
	TypeRef ret_type;
	if (IsBaseType()) {
		for (int i = 0; i < m_size && ret_type.IsNull(); ++i) {
			if (name.Compare(mem_names[i], true)) {
				ret_type.New();
				ret_type->m_name    = mem_names[i];
				ret_type->m_size    = 1;
				ret_type->m_offset  = i;
				ret_type->m_variant = m_variant;
			}
		}
	} else {
		for (int i = 0; i < m_members.GetSize() && ret_type.IsNull(); ++i) {
			if (m_members[i]->m_name.Compare(name, true)) {
				ret_type = m_members[i];
			}
		}
	}
	return ret_type;
}

class TypeList
{
private:
	mtlStringMap<TypeRef> m_map;

public:
	TypeRef GetType(const mtlChars &name) const;
};

TypeRef TypeList::GetType(const mtlChars &name) const
{
	mtlList<mtlChars> names;
	name.SplitByChar(names, '.');
	const TypeRef *found = m_map.GetEntry(names.GetFirst()->GetItem());
	TypeRef type = found != NULL ? *found : TypeRef();
	mtlItem<mtlChars> *i = names.GetFirst()->GetNext();
	while (i != NULL && !type.IsNull()) {
		type = type->GetMember(i->GetItem());
		i = i->GetNext();
	}
	return type;
}

class ScopedTypeList
{
private:
	mtlList<TypeList> m_scopes;

public:
	void    PushScope( void ) { m_scopes.AddLast(); }
	void    PopScope( void );
	TypeRef GetType(const mtlChars &name) const;
};

void ScopedTypeList::PopScope( void )
{
	if (m_scopes.GetSize() > 0) {
		m_scopes.RemoveLast();
	}
}

TypeRef ScopedTypeList::GetType(const mtlChars &name) const
{
	const mtlItem<TypeList> *i = m_scopes.GetLast();
	TypeRef type;
	while (i != NULL && type.IsNull()) {
		type = i->GetItem().GetType(name);
		i = i->GetPrev();
	}
	return type;
}

class IntermediateCompilationData
{
private:
	mtlList<swsl::Instruction>  m_program;
	swsl::Shader               *m_output;
	int                         m_main_count;

public:
	void Initialize( void ) {}
};

void RemoveComments(const mtlChars &input, mtlString &output)
{
	output.Free();
	output.Reserve(input.GetSize());
	int i = 0;
	mtlChars r = input;
	while (r.GetSize() > 0) {

		int a  = r.FindFirstString("//");
		a = a < 0 ? input.GetSize() : a;
		int b = r.FindFirstString("/*");
		b = b < 0 ? input.GetSize() : b;

		if (a < b) {
			output.Append(mtlChars(r, i, a));
			i = r.FindFirstChar("\n\r");
			i = i < 0 ? input.GetSize() : i + 1;
		} else if (b < a) {
			output.Append(mtlChars(r, i, b));
			i = r.FindFirstString("*/");
			i = i < 0 ? input.GetSize() : i + 2;
		}

		r = mtlChars(input, i, input.GetSize());
	}
}

bool Compiler::Compile(const mtlChars &input, swsl::Shader &output)
{
	mtlString code;
	RemoveComments(input, code);
	//return inst.CompileScope(code, output);
	return false;
}
