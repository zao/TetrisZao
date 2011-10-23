#pragma once

struct sprite_vertex {
	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex;
};

struct stream_source {
	u32 offset, stride;
	IDirect3DVertexBuffer9Ptr buffer;
};

struct static_art_description {
	static_art_description() {}
	static_art_description(int x, int y, int z, int w, int h, std::wstring filename)
		: x(x), y(y), z(z), w(w), h(h), filename(filename) {}
	int x, y;
	int z;
	int w, h;
	std::wstring filename;
	boost::optional<std::vector<D3DCOLOR> > color_source;
};

struct static_art {
	D3DXMATRIX transform;
	typedef std::map<u8, stream_source> stream_map;
	stream_map streams;
	IDirect3DVertexDeclaration9Ptr decl;
	IDirect3DTexture9Ptr tex;
};

typedef std::multimap<int, static_art, std::greater<int> > static_art_map;

std::vector<sprite_vertex> make_sprite_vertices(int w, int h);
static_art compile_static_art(IDirect3DDevice9Ptr dev, static_art_description const& sad);
void draw_static_art(IDirect3DDevice9Ptr& dev, std::pair<int const, static_art>& sap);