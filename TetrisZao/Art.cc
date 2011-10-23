#include "Pch.h"
#include "Art.h"
#include "Helpers.h"

std::vector<sprite_vertex> make_sprite_vertices(int w, int h) {
	sprite_vertex tmp[] = {
		{ D3DXVECTOR3(-w/2.0f, -h/2.0f, 0.0f), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3(-w/2.0f,  h/2.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR3( w/2.0f,  h/2.0f, 0.0f), D3DXVECTOR2(1.0f, 0.0f) },
		{ D3DXVECTOR3( w/2.0f, -h/2.0f, 0.0f), D3DXVECTOR2(1.0f, 1.0f) },
	};
	return std::vector<sprite_vertex>(tmp, tmp + 4);
}

static_art compile_static_art(IDirect3DDevice9Ptr dev, static_art_description const& sad) {
	static_art sa;

	// Construct translational transform
	D3DXMatrixTranslation(&sa.transform, (float)sad.x, (float)sad.y, (float)sad.z);

	// Load texture image
	D3DXCreateTextureFromFile(dev, sad.filename.c_str(), &sa.tex);

	// Prepare stream source data
	std::vector<sprite_vertex> vertices = make_sprite_vertices(sad.w, sad.h);
	std::vector<D3DCOLOR> default_color(vertices.size(), 0xFFFFFFFFU);
	std::vector<D3DCOLOR> const & colors = sad.color_source ? *sad.color_source : default_color;

	stream_source vertex_source = { 0, sizeof(sprite_vertex) };
	stream_source color_source = { 0, sizeof(D3DCOLOR) };

	// Create stream sources
	dev->CreateVertexBuffer(sizeof(sprite_vertex)*vertices.size(), D3DUSAGE_WRITEONLY,
		0, D3DPOOL_MANAGED, &vertex_source.buffer, 0);
	dev->CreateVertexBuffer(sizeof(D3DCOLOR)*colors.size(), D3DUSAGE_WRITEONLY,
		0, D3DPOOL_MANAGED, &color_source.buffer, 0);

	// Fill stream sources
	std::copy(vertices.begin(), vertices.end(),
		dx::vertex_lock<sprite_vertex>::type(vertex_source.buffer).data());

	std::copy(colors.begin(), colors.end(),
		dx::vertex_lock<D3DCOLOR>::type(color_source.buffer).data());

	sa.streams[0] = vertex_source;
	sa.streams[1] = color_source;

	// Build vertex declaration
	D3DVERTEXELEMENT9 static_decl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 1, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		D3DDECL_END()
	};
	dev->CreateVertexDeclaration(static_decl, &sa.decl);

	return sa;
}

void draw_static_art(IDirect3DDevice9Ptr& dev, std::pair<int const, static_art>& sap) {
	static_art & sa = sap.second;
	BOOST_FOREACH(static_art::stream_map::value_type & stream, sa.streams) {
		stream_source & ss = stream.second;
		dev->SetStreamSource(stream.first, ss.buffer, ss.offset, ss.stride);
	}
	dev->SetTexture(0, sa.tex);
	dev->SetTransform(D3DTS_WORLD, &sa.transform);
	dev->SetVertexDeclaration(sa.decl);
	dev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
}