/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "log.hpp"
#include "d3d11_runtime.hpp"
#include "d3d11_effect_compiler.hpp"
#include "effect_lexer.hpp"
#include "input.hpp"
#include "resource_loading.hpp"
#include <imgui.h>
#include <algorithm>

#include "JWEMSingleton.h"

namespace reshade::d3d11
{
	extern DXGI_FORMAT make_format_srgb(DXGI_FORMAT format);
	extern DXGI_FORMAT make_format_normal(DXGI_FORMAT format);
	extern DXGI_FORMAT make_format_typeless(DXGI_FORMAT format);

	d3d11_runtime::d3d11_runtime(ID3D11Device *device, IDXGISwapChain *swapchain) :
		runtime(device->GetFeatureLevel()), _device(device), _swapchain(swapchain),
		_stateblock(device)
	{
		assert(device != nullptr);
		assert(swapchain != nullptr);

		_device->GetImmediateContext(&_immediate_context);

		HRESULT hr;
		DXGI_ADAPTER_DESC adapter_desc;
		com_ptr<IDXGIDevice> dxgidevice;
		com_ptr<IDXGIAdapter> dxgiadapter;

		hr = _device->QueryInterface(&dxgidevice);

		assert(SUCCEEDED(hr));

		hr = dxgidevice->GetAdapter(&dxgiadapter);

		assert(SUCCEEDED(hr));

		hr = dxgiadapter->GetDesc(&adapter_desc);

		assert(SUCCEEDED(hr));

		_vendor_id = adapter_desc.VendorId;
		_device_id = adapter_desc.DeviceId;

		subscribe_to_menu("DX11", [this]() { draw_debug_menu(); });
		subscribe_to_load_config([this](const ini_file& config) {
			config.get("DX11_BUFFER_DETECTION", "DepthBufferRetrievalMode", depth_buffer_before_clear);
			config.get("DX11_BUFFER_DETECTION", "DepthBufferTextureFormat", depth_buffer_texture_format);
			config.get("DX11_BUFFER_DETECTION", "ExtendedDepthBufferDetection", extended_depth_buffer_detection);
			config.get("DX11_BUFFER_DETECTION", "DepthBufferClearingNumber", cleared_depth_buffer_index);
		});
		subscribe_to_save_config([this](ini_file& config) {
			config.set("DX11_BUFFER_DETECTION", "DepthBufferRetrievalMode", depth_buffer_before_clear);
			config.set("DX11_BUFFER_DETECTION", "DepthBufferTextureFormat", depth_buffer_texture_format);
			config.set("DX11_BUFFER_DETECTION", "ExtendedDepthBufferDetection", extended_depth_buffer_detection);
			config.set("DX11_BUFFER_DETECTION", "DepthBufferClearingNumber", cleared_depth_buffer_index);
		});
	}

	bool d3d11_runtime::init_backbuffer_texture()
	{
		// Get back buffer texture
		HRESULT hr = _swapchain->GetBuffer(0, IID_PPV_ARGS(&_backbuffer));

		assert(SUCCEEDED(hr));

		D3D11_TEXTURE2D_DESC texdesc = { };
		texdesc.Width = _width;
		texdesc.Height = _height;
		texdesc.ArraySize = texdesc.MipLevels = 1;
		texdesc.Format = make_format_typeless(_backbuffer_format);
		texdesc.SampleDesc = { 1, 0 };
		texdesc.Usage = D3D11_USAGE_DEFAULT;
		texdesc.BindFlags = D3D11_BIND_RENDER_TARGET;

		OSVERSIONINFOEX verinfo_windows7 = { sizeof(OSVERSIONINFOEX), 6, 1 };
		const bool is_windows7 = VerifyVersionInfo(&verinfo_windows7, VER_MAJORVERSION | VER_MINORVERSION,
			VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_MINORVERSION, VER_EQUAL)) != FALSE;

		if (_is_multisampling_enabled ||
			make_format_normal(_backbuffer_format) != _backbuffer_format ||
			!is_windows7)
		{
			hr = _device->CreateTexture2D(&texdesc, nullptr, &_backbuffer_resolved);

			if (FAILED(hr))
			{
				LOG(ERROR) << "Failed to create back buffer resolve texture ("
					"Width = " << texdesc.Width << ", "
					"Height = " << texdesc.Height << ", "
					"Format = " << texdesc.Format << ", "
					"SampleCount = " << texdesc.SampleDesc.Count << ", "
					"SampleQuality = " << texdesc.SampleDesc.Quality << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
				return false;
			}

			hr = _device->CreateRenderTargetView(_backbuffer.get(), nullptr, &_backbuffer_rtv[2]);

			assert(SUCCEEDED(hr));
		}
		else
		{
			_backbuffer_resolved = _backbuffer;
		}

		// Create back buffer shader texture
		texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		hr = _device->CreateTexture2D(&texdesc, nullptr, &_backbuffer_texture);

		if (SUCCEEDED(hr))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc = { };
			srvdesc.Format = make_format_normal(texdesc.Format);
			srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvdesc.Texture2D.MipLevels = texdesc.MipLevels;

			if (SUCCEEDED(hr))
			{
				hr = _device->CreateShaderResourceView(_backbuffer_texture.get(), &srvdesc, &_backbuffer_texture_srv[0]);
			}
			else
			{
				LOG(ERROR) << "Failed to create back buffer texture resource view ("
					"Format = " << srvdesc.Format << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
			}

			srvdesc.Format = make_format_srgb(texdesc.Format);

			if (SUCCEEDED(hr))
			{
				hr = _device->CreateShaderResourceView(_backbuffer_texture.get(), &srvdesc, &_backbuffer_texture_srv[1]);
			}
			else
			{
				LOG(ERROR) << "Failed to create back buffer SRGB texture resource view ("
					"Format = " << srvdesc.Format << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
			}
		}
		else
		{
			LOG(ERROR) << "Failed to create back buffer texture ("
				"Width = " << texdesc.Width << ", "
				"Height = " << texdesc.Height << ", "
				"Format = " << texdesc.Format << ", "
				"SampleCount = " << texdesc.SampleDesc.Count << ", "
				"SampleQuality = " << texdesc.SampleDesc.Quality << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
		}

		if (FAILED(hr))
		{
			return false;
		}

		D3D11_RENDER_TARGET_VIEW_DESC rtdesc = { };
		rtdesc.Format = make_format_normal(texdesc.Format);
		rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = _device->CreateRenderTargetView(_backbuffer_resolved.get(), &rtdesc, &_backbuffer_rtv[0]);

		if (FAILED(hr))
		{
			LOG(ERROR) << "Failed to create back buffer render target ("
				"Format = " << rtdesc.Format << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
			return false;
		}

		rtdesc.Format = make_format_srgb(texdesc.Format);

		hr = _device->CreateRenderTargetView(_backbuffer_resolved.get(), &rtdesc, &_backbuffer_rtv[1]);

		if (FAILED(hr))
		{
			LOG(ERROR) << "Failed to create back buffer SRGB render target ("
				"Format = " << rtdesc.Format << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
			return false;
		}

		{
			const resources::data_resource vs = resources::load_data_resource(IDR_RCDATA1);

			hr = _device->CreateVertexShader(vs.data, vs.data_size, nullptr, &_copy_vertex_shader);

			if (FAILED(hr))
			{
				return false;
			}

			const resources::data_resource ps = resources::load_data_resource(IDR_RCDATA2);

			hr = _device->CreatePixelShader(ps.data, ps.data_size, nullptr, &_copy_pixel_shader);

			if (FAILED(hr))
			{
				return false;
			}
		}

		{
			const D3D11_SAMPLER_DESC desc = {
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP
			};

			hr = _device->CreateSamplerState(&desc, &_copy_sampler);

			if (FAILED(hr))
			{
				return false;
			}
		}

		return true;
	}
	bool d3d11_runtime::init_default_depth_stencil()
	{
		const D3D11_TEXTURE2D_DESC texdesc = {
			_width,
			_height,
			1, 1,
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ 1, 0 },
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_DEPTH_STENCIL
		};

		com_ptr<ID3D11Texture2D> depth_stencil_texture;

		HRESULT hr = _device->CreateTexture2D(&texdesc, nullptr, &depth_stencil_texture);

		if (FAILED(hr))
		{
			LOG(ERROR) << "Failed to create depth stencil texture ("
				"Width = " << texdesc.Width << ", "
				"Height = " << texdesc.Height << ", "
				"Format = " << texdesc.Format << ", "
				"SampleCount = " << texdesc.SampleDesc.Count << ", "
				"SampleQuality = " << texdesc.SampleDesc.Quality << ")! HRESULT is '" << std::hex << hr << std::dec << "'.";
			return false;
		}

		hr = _device->CreateDepthStencilView(depth_stencil_texture.get(), nullptr, &_default_depthstencil);

		return SUCCEEDED(hr);
	}
	bool d3d11_runtime::init_fx_resources()
	{
		D3D11_RASTERIZER_DESC desc = { };
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;

		return SUCCEEDED(_device->CreateRasterizerState(&desc, &_effect_rasterizer_state));
	}
	bool d3d11_runtime::init_imgui_resources()
	{
		HRESULT hr = E_FAIL;

		// Create the vertex shader
		{
			const resources::data_resource vs = resources::load_data_resource(IDR_RCDATA3);

			hr = _device->CreateVertexShader(vs.data, vs.data_size, nullptr, &_imgui_vertex_shader);

			if (FAILED(hr))
			{
				return false;
			}

			// Create the input layout
			const D3D11_INPUT_ELEMENT_DESC input_layout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(ImDrawVert, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			hr = _device->CreateInputLayout(input_layout, _countof(input_layout), vs.data, vs.data_size, &_imgui_input_layout);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create the pixel shader
		{
			const resources::data_resource ps = resources::load_data_resource(IDR_RCDATA4);

			hr = _device->CreatePixelShader(ps.data, ps.data_size, nullptr, &_imgui_pixel_shader);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create the constant buffer
		{
			D3D11_BUFFER_DESC desc = { };
			desc.ByteWidth = 16 * sizeof(float);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			hr = _device->CreateBuffer(&desc, nullptr, &_imgui_constant_buffer);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create the blending setup
		{
			D3D11_BLEND_DESC desc = { };
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			hr = _device->CreateBlendState(&desc, &_imgui_blend_state);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create the depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc = { };
			desc.DepthEnable = false;
			desc.StencilEnable = false;

			hr = _device->CreateDepthStencilState(&desc, &_imgui_depthstencil_state);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create the rasterizer state
		{
			D3D11_RASTERIZER_DESC desc = { };
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_NONE;
			desc.ScissorEnable = true;
			desc.DepthClipEnable = true;

			hr = _device->CreateRasterizerState(&desc, &_imgui_rasterizer_state);

			if (FAILED(hr))
			{
				return false;
			}
		}

		// Create texture sampler
		{
			D3D11_SAMPLER_DESC desc = { };
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

			hr = _device->CreateSamplerState(&desc, &_imgui_texture_sampler);

			if (FAILED(hr))
			{
				return false;
			}
		}

		return true;
	}
	bool d3d11_runtime::init_imgui_font_atlas()
	{
		int width, height;
		unsigned char *pixels;

		ImGui::SetCurrentContext(_imgui_context);
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		const D3D11_TEXTURE2D_DESC tex_desc = {
			static_cast<UINT>(width),
			static_cast<UINT>(height),
			1, 1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			{ 1, 0 },
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE
		};
		const D3D11_SUBRESOURCE_DATA tex_data = {
			pixels,
			tex_desc.Width * 4
		};

		com_ptr<ID3D11Texture2D> font_atlas;
		com_ptr<ID3D11ShaderResourceView> font_atlas_view;

		HRESULT hr = _device->CreateTexture2D(&tex_desc, &tex_data, &font_atlas);

		if (FAILED(hr))
		{
			return false;
		}

		hr = _device->CreateShaderResourceView(font_atlas.get(), nullptr, &font_atlas_view);

		if (FAILED(hr))
		{
			return false;
		}

		d3d11_tex_data obj = { };
		obj.texture = font_atlas;
		obj.srv[0] = font_atlas_view;

		_imgui_font_atlas_texture = std::make_unique<d3d11_tex_data>(obj);

		return true;
	}

	bool d3d11_runtime::on_init(const DXGI_SWAP_CHAIN_DESC &desc)
	{
		_width = desc.BufferDesc.Width;
		_height = desc.BufferDesc.Height;
		_backbuffer_format = desc.BufferDesc.Format;
		_is_multisampling_enabled = desc.SampleDesc.Count > 1;
		_input = input::register_window(desc.OutputWindow);

		if (!init_backbuffer_texture() ||
			!init_default_depth_stencil() ||
			!init_fx_resources() ||
			!init_imgui_resources() ||
			!init_imgui_font_atlas())
		{
			return false;
		}

		// Clear reference count to make UnrealEngine happy
		_backbuffer->Release();

		return runtime::on_init();
	}
	void d3d11_runtime::on_reset()
	{
		if (!is_initialized())
		{
			return;
		}

		runtime::on_reset();

		// Reset reference count to make UnrealEngine happy
		_backbuffer->AddRef();

		// Destroy resources
		_backbuffer.reset();
		_backbuffer_resolved.reset();
		_backbuffer_texture.reset();
		_backbuffer_texture_srv[0].reset();
		_backbuffer_texture_srv[1].reset();
		_backbuffer_rtv[0].reset();
		_backbuffer_rtv[1].reset();
		_backbuffer_rtv[2].reset();

		_depthstencil.reset();
		_depthstencil_replacement.reset();
		_depthstencil_texture.reset();
		_depthstencil_texture_srv.reset();

		_depth_texture_saves.clear();

		_default_depthstencil.reset();
		_copy_vertex_shader.reset();
		_copy_pixel_shader.reset();
		_copy_sampler.reset();

		_effect_rasterizer_state.reset();

		_imgui_vertex_buffer.reset();
		_imgui_index_buffer.reset();
		_imgui_vertex_shader.reset();
		_imgui_pixel_shader.reset();
		_imgui_input_layout.reset();
		_imgui_constant_buffer.reset();
		_imgui_texture_sampler.reset();
		_imgui_rasterizer_state.reset();
		_imgui_blend_state.reset();
		_imgui_depthstencil_state.reset();
		_imgui_vertex_buffer_size = 0;
		_imgui_index_buffer_size = 0;
	}
	void d3d11_runtime::on_reset_effect()
	{
		runtime::on_reset_effect();

		_effect_sampler_descs.clear();
		_effect_sampler_states.clear();
		_constant_buffers.clear();

		_effect_shader_resources.resize(3);
		_effect_shader_resources[0] = _backbuffer_texture_srv[0];
		_effect_shader_resources[1] = _backbuffer_texture_srv[1];
		_effect_shader_resources[2] = _depthstencil_texture_srv;
	}
	void d3d11_runtime::on_present(draw_call_tracker &tracker)
	{
		if (!is_initialized())
			return;

		auto presentStart = std::chrono::high_resolution_clock::now();

		Singleton * JWEmSingleton = Singleton::getInstance();

		_vertices = tracker.total_vertices();
		_drawcalls = tracker.total_drawcalls();

		_current_tracker = tracker;

#if RESHADE_DX11_CAPTURE_DEPTH_BUFFERS
		detect_depth_source(tracker);
#endif

		// Evaluate queries
		for (technique &technique : _techniques)
		{
			d3d11_technique_data &technique_data = *technique.impl->as<d3d11_technique_data>();

			if (technique.enabled && technique_data.query_in_flight)
			{
				UINT64 timestamp0, timestamp1;
				D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;

				if (_immediate_context->GetData(technique_data.timestamp_disjoint.get(), &disjoint_data, sizeof(disjoint_data), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
					_immediate_context->GetData(technique_data.timestamp_query_beg.get(), &timestamp0, sizeof(timestamp0), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
					_immediate_context->GetData(technique_data.timestamp_query_end.get(), &timestamp1, sizeof(timestamp1), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK)
				{
					if (!disjoint_data.Disjoint)
						technique.average_gpu_duration.append((timestamp1 - timestamp0) * 1'000'000'000 / disjoint_data.Frequency);
					technique_data.query_in_flight = false;
				}
			}
		}

		// Capture device state
		_stateblock.capture(_immediate_context.get());

		// Disable unused pipeline stages
		_immediate_context->HSSetShader(nullptr, nullptr, 0);
		_immediate_context->DSSetShader(nullptr, nullptr, 0);
		_immediate_context->GSSetShader(nullptr, nullptr, 0);

		// Resolve back buffer
		if (_backbuffer_resolved != _backbuffer)
		{
			_immediate_context->ResolveSubresource(_backbuffer_resolved.get(), 0, _backbuffer.get(), 0, _backbuffer_format);
		}

		// Apply post processing
		if (is_effect_loaded())
		{
			// Setup real back buffer
			const auto rtv = _backbuffer_rtv[0].get();
			_immediate_context->OMSetRenderTargets(1, &rtv, nullptr);

			// Setup vertex input
			const uintptr_t null = 0;
			_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_immediate_context->IASetInputLayout(nullptr);
			_immediate_context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer *const *>(&null), reinterpret_cast<const UINT *>(&null), reinterpret_cast<const UINT *>(&null));

			_immediate_context->RSSetState(_effect_rasterizer_state.get());

			// Setup samplers
			_immediate_context->VSSetSamplers(0, static_cast<UINT>(_effect_sampler_states.size()), reinterpret_cast<ID3D11SamplerState *const *>(_effect_sampler_states.data()));
			_immediate_context->PSSetSamplers(0, static_cast<UINT>(_effect_sampler_states.size()), reinterpret_cast<ID3D11SamplerState *const *>(_effect_sampler_states.data()));

			on_present_effect();
		}

		// Apply presenting
		runtime::on_present();

		// Copy to back buffer
		if (_backbuffer_resolved != _backbuffer)
		{
			_immediate_context->CopyResource(_backbuffer_texture.get(), _backbuffer_resolved.get());

			const auto rtv = _backbuffer_rtv[2].get();
			_immediate_context->OMSetRenderTargets(1, &rtv, nullptr);

			const uintptr_t null = 0;
			_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_immediate_context->IASetInputLayout(nullptr);
			_immediate_context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer *const *>(&null), reinterpret_cast<const UINT *>(&null), reinterpret_cast<const UINT *>(&null));

			_immediate_context->RSSetState(_effect_rasterizer_state.get());

			_immediate_context->VSSetShader(_copy_vertex_shader.get(), nullptr, 0);
			_immediate_context->PSSetShader(_copy_pixel_shader.get(), nullptr, 0);
			const auto sst = _copy_sampler.get();
			_immediate_context->PSSetSamplers(0, 1, &sst);
			const auto srv = _backbuffer_texture_srv[make_format_srgb(_backbuffer_format) == _backbuffer_format].get();
			_immediate_context->PSSetShaderResources(0, 1, &srv);

			_immediate_context->Draw(3, 0);
		}

		// Apply previous device state
		_stateblock.apply_and_release();

		auto presentEnd = std::chrono::high_resolution_clock::now();
		JWEmSingleton->lastPresentDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(presentEnd - presentStart).count();

	}

	void d3d11_runtime::capture_frame(uint8_t *buffer) const
	{
		if (_backbuffer_format != DXGI_FORMAT_R8G8B8A8_UNORM &&
			_backbuffer_format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
			_backbuffer_format != DXGI_FORMAT_B8G8R8A8_UNORM &&
			_backbuffer_format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
		{
			LOG(WARNING) << "Screenshots are not supported for back buffer format " << _backbuffer_format << ".";
			return;
		}

		D3D11_TEXTURE2D_DESC texture_desc = { };
		texture_desc.Width = _width;
		texture_desc.Height = _height;
		texture_desc.ArraySize = 1;
		texture_desc.MipLevels = 1;
		texture_desc.Format = _backbuffer_format;
		texture_desc.SampleDesc.Count = 1;
		texture_desc.Usage = D3D11_USAGE_STAGING;
		texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		com_ptr<ID3D11Texture2D> texture_staging;

		HRESULT hr = _device->CreateTexture2D(&texture_desc, nullptr, &texture_staging);

		if (FAILED(hr))
		{
			LOG(ERROR) << "Failed to create staging resource for screenshot capture! HRESULT is '" << std::hex << hr << std::dec << "'.";
			return;
		}

		_immediate_context->CopyResource(texture_staging.get(), _backbuffer_resolved.get());

		D3D11_MAPPED_SUBRESOURCE mapped;
		hr = _immediate_context->Map(texture_staging.get(), 0, D3D11_MAP_READ, 0, &mapped);

		if (FAILED(hr))
		{
			LOG(ERROR) << "Failed to map staging resource with screenshot capture! HRESULT is '" << std::hex << hr << std::dec << "'.";
			return;
		}

		auto mapped_data = static_cast<BYTE *>(mapped.pData);
		const UINT pitch = texture_desc.Width * 4;

		for (UINT y = 0; y < texture_desc.Height; y++)
		{
			CopyMemory(buffer, mapped_data, std::min(pitch, static_cast<UINT>(mapped.RowPitch)));

			for (UINT x = 0; x < pitch; x += 4)
			{
				buffer[x + 3] = 0xFF;

				if (texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
				{
					std::swap(buffer[x + 0], buffer[x + 2]);
				}
			}

			buffer += pitch;
			mapped_data += mapped.RowPitch;
		}

		_immediate_context->Unmap(texture_staging.get(), 0);
	}
	bool d3d11_runtime::load_effect(const reshadefx::syntax_tree &ast, std::string &errors)
	{
		return d3d11_effect_compiler(this, ast, errors, false).run();
	}
	bool d3d11_runtime::update_texture(texture &texture, const uint8_t *data)
	{
		if (texture.impl_reference != texture_reference::none)
		{
			return false;
		}

		const auto texture_impl = texture.impl->as<d3d11_tex_data>();

		assert(data != nullptr);
		assert(texture_impl != nullptr);

		switch (texture.format)
		{
			case texture_format::r8:
			{
				std::vector<uint8_t> data2(texture.width * texture.height);
				for (size_t i = 0, k = 0; i < texture.width * texture.height * 4; i += 4, k++)
					data2[k] = data[i];
				_immediate_context->UpdateSubresource(texture_impl->texture.get(), 0, nullptr, data2.data(), texture.width, texture.width * texture.height);
				break;
			}
			case texture_format::rg8:
			{
				std::vector<uint8_t> data2(texture.width * texture.height * 2);
				for (size_t i = 0, k = 0; i < texture.width * texture.height * 4; i += 4, k += 2)
					data2[k] = data[i],
					data2[k + 1] = data[i + 1];
				_immediate_context->UpdateSubresource(texture_impl->texture.get(), 0, nullptr, data2.data(), texture.width * 2, texture.width * texture.height * 2);
				break;
			}
			default:
			{
				_immediate_context->UpdateSubresource(texture_impl->texture.get(), 0, nullptr, data, texture.width * 4, texture.width * texture.height * 4);
				break;
			}
		}

		if (texture.levels > 1)
		{
			_immediate_context->GenerateMips(texture_impl->srv[0].get());
		}

		return true;
	}

	void d3d11_runtime::render_technique(const technique &technique)
	{
		d3d11_technique_data &technique_data = *technique.impl->as<d3d11_technique_data>();

		if (!technique_data.query_in_flight)
		{
			_immediate_context->Begin(technique_data.timestamp_disjoint.get());
			_immediate_context->End(technique_data.timestamp_query_beg.get());
		}

		bool is_default_depthstencil_cleared = false;

		// Setup shader constants
		if (technique.uniform_storage_index >= 0)
		{
			const auto constant_buffer = _constant_buffers[technique.uniform_storage_index].get();
			D3D11_MAPPED_SUBRESOURCE mapped;

			const HRESULT hr = _immediate_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

			if (SUCCEEDED(hr))
			{
				CopyMemory(mapped.pData, get_uniform_value_storage().data() + technique.uniform_storage_offset, mapped.RowPitch);

				_immediate_context->Unmap(constant_buffer, 0);
			}
			else
			{
				LOG(ERROR) << "Failed to map constant buffer! HRESULT is '" << std::hex << hr << std::dec << "'!";
			}

			_immediate_context->VSSetConstantBuffers(0, 1, &constant_buffer);
			_immediate_context->PSSetConstantBuffers(0, 1, &constant_buffer);
		}

		for (const auto &pass_object : technique.passes)
		{
			const d3d11_pass_data &pass = *pass_object->as<d3d11_pass_data>();

			// Setup states
			_immediate_context->VSSetShader(pass.vertex_shader.get(), nullptr, 0);
			_immediate_context->PSSetShader(pass.pixel_shader.get(), nullptr, 0);

			_immediate_context->OMSetBlendState(pass.blend_state.get(), nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			_immediate_context->OMSetDepthStencilState(pass.depth_stencil_state.get(), pass.stencil_reference);

			// Save back buffer of previous pass
			_immediate_context->CopyResource(_backbuffer_texture.get(), _backbuffer_resolved.get());

			// Setup shader resources
			_immediate_context->VSSetShaderResources(0, static_cast<UINT>(pass.shader_resources.size()), reinterpret_cast<ID3D11ShaderResourceView *const *>(pass.shader_resources.data()));
			_immediate_context->PSSetShaderResources(0, static_cast<UINT>(pass.shader_resources.size()), reinterpret_cast<ID3D11ShaderResourceView *const *>(pass.shader_resources.data()));

			// Setup render targets
			if (static_cast<UINT>(pass.viewport.Width) == _width && static_cast<UINT>(pass.viewport.Height) == _height)
			{
				_immediate_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, reinterpret_cast<ID3D11RenderTargetView *const *>(pass.render_targets), _default_depthstencil.get());

				if (!is_default_depthstencil_cleared)
				{
					is_default_depthstencil_cleared = true;

					_immediate_context->ClearDepthStencilView(_default_depthstencil.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				}
			}
			else
			{
				_immediate_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, reinterpret_cast<ID3D11RenderTargetView *const *>(pass.render_targets), nullptr);
			}

			_immediate_context->RSSetViewports(1, &pass.viewport);

			if (pass.clear_render_targets)
			{
				for (const auto &target : pass.render_targets)
				{
					if (target != nullptr)
					{
						const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
						_immediate_context->ClearRenderTargetView(target.get(), color);
					}
				}
			}

			// Draw triangle
			_immediate_context->Draw(3, 0);

			_vertices += 3;
			_drawcalls += 1;

			// Reset render targets
			_immediate_context->OMSetRenderTargets(0, nullptr, nullptr);

			// Reset shader resources
			ID3D11ShaderResourceView *null[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
			_immediate_context->VSSetShaderResources(0, static_cast<UINT>(pass.shader_resources.size()), null);
			_immediate_context->PSSetShaderResources(0, static_cast<UINT>(pass.shader_resources.size()), null);

			// Update shader resources
			for (const auto &resource : pass.render_target_resources)
			{
				if (resource == nullptr)
				{
					continue;
				}

				D3D11_SHADER_RESOURCE_VIEW_DESC resource_desc;
				resource->GetDesc(&resource_desc);

				if (resource_desc.Texture2D.MipLevels > 1)
				{
					_immediate_context->GenerateMips(resource.get());
				}
			}
		}

		if (!technique_data.query_in_flight)
		{
			_immediate_context->End(technique_data.timestamp_query_end.get());
			_immediate_context->End(technique_data.timestamp_disjoint.get());
			technique_data.query_in_flight = true;
		}
	}
	void d3d11_runtime::render_imgui_draw_data(ImDrawData *draw_data)
	{
		// Create and grow vertex/index buffers if needed
		if (_imgui_vertex_buffer == nullptr ||
			_imgui_vertex_buffer_size < draw_data->TotalVtxCount)
		{
			_imgui_vertex_buffer.reset();
			_imgui_vertex_buffer_size = draw_data->TotalVtxCount + 5000;

			D3D11_BUFFER_DESC desc = { };
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = _imgui_vertex_buffer_size * sizeof(ImDrawVert);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;

			if (FAILED(_device->CreateBuffer(&desc, nullptr, &_imgui_vertex_buffer)))
			{
				return;
			}
		}
		if (_imgui_index_buffer == nullptr ||
			_imgui_index_buffer_size < draw_data->TotalIdxCount)
		{
			_imgui_index_buffer.reset();
			_imgui_index_buffer_size = draw_data->TotalIdxCount + 10000;

			D3D11_BUFFER_DESC desc = { };
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = _imgui_index_buffer_size * sizeof(ImDrawIdx);
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			if (FAILED(_device->CreateBuffer(&desc, nullptr, &_imgui_index_buffer)))
			{
				return;
			}
		}

		D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;

		if (FAILED(_immediate_context->Map(_imgui_vertex_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource)) ||
			FAILED(_immediate_context->Map(_imgui_index_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource)))
		{
			return;
		}

		auto vtx_dst = static_cast<ImDrawVert *>(vtx_resource.pData);
		auto idx_dst = static_cast<ImDrawIdx *>(idx_resource.pData);

		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList *const cmd_list = draw_data->CmdLists[n];

			CopyMemory(vtx_dst, &cmd_list->VtxBuffer.front(), cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
			CopyMemory(idx_dst, &cmd_list->IdxBuffer.front(), cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));

			vtx_dst += cmd_list->VtxBuffer.size();
			idx_dst += cmd_list->IdxBuffer.size();
		}

		_immediate_context->Unmap(_imgui_vertex_buffer.get(), 0);
		_immediate_context->Unmap(_imgui_index_buffer.get(), 0);

		// Setup orthographic projection matrix
		D3D11_MAPPED_SUBRESOURCE constant_buffer_data;

		if (FAILED(_immediate_context->Map(_imgui_constant_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &constant_buffer_data)))
		{
			return;
		}

		const float ortho_projection[16] = {
			2.0f / _width, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f / _height, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			-1.0f, 1.0f, 0.5f, 1.0f
		};

		CopyMemory(constant_buffer_data.pData, ortho_projection, sizeof(ortho_projection));

		_immediate_context->Unmap(_imgui_constant_buffer.get(), 0);

		// Setup render state
		const auto render_target = _backbuffer_rtv[0].get();
		_immediate_context->OMSetRenderTargets(1, &render_target, nullptr);

		const D3D11_VIEWPORT viewport = { 0, 0, static_cast<float>(_width), static_cast<float>(_height), 0.0f, 1.0f };
		_immediate_context->RSSetViewports(1, &viewport);

		const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
		_immediate_context->OMSetBlendState(_imgui_blend_state.get(), blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
		_immediate_context->OMSetDepthStencilState(_imgui_depthstencil_state.get(), 0);
		_immediate_context->RSSetState(_imgui_rasterizer_state.get());

		UINT stride = sizeof(ImDrawVert), offset = 0;
		ID3D11Buffer *vertex_buffers[1] = { _imgui_vertex_buffer.get() };
		ID3D11Buffer *constant_buffers[1] = { _imgui_constant_buffer.get() };
		ID3D11SamplerState *samplers[1] = { _imgui_texture_sampler.get() };
		_immediate_context->IASetInputLayout(_imgui_input_layout.get());
		_immediate_context->IASetVertexBuffers(0, 1, vertex_buffers, &stride, &offset);
		_immediate_context->IASetIndexBuffer(_imgui_index_buffer.get(), sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
		_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_immediate_context->VSSetShader(_imgui_vertex_shader.get(), nullptr, 0);
		_immediate_context->VSSetConstantBuffers(0, 1, constant_buffers);
		_immediate_context->PSSetShader(_imgui_pixel_shader.get(), nullptr, 0);
		_immediate_context->PSSetSamplers(0, 1, samplers);

		// Render command lists
		UINT vtx_offset = 0, idx_offset = 0;

		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList *const cmd_list = draw_data->CmdLists[n];

			for (const ImDrawCmd *cmd = cmd_list->CmdBuffer.begin(); cmd != cmd_list->CmdBuffer.end(); idx_offset += cmd->ElemCount, cmd++)
			{
				const D3D11_RECT scissor_rect = {
					static_cast<LONG>(cmd->ClipRect.x),
					static_cast<LONG>(cmd->ClipRect.y),
					static_cast<LONG>(cmd->ClipRect.z),
					static_cast<LONG>(cmd->ClipRect.w)
				};

				ID3D11ShaderResourceView *const texture_view = static_cast<const d3d11_tex_data *>(cmd->TextureId)->srv[0].get();
				_immediate_context->PSSetShaderResources(0, 1, &texture_view);
				_immediate_context->RSSetScissorRects(1, &scissor_rect);

				_immediate_context->DrawIndexed(cmd->ElemCount, idx_offset, vtx_offset);
			}

			vtx_offset += cmd_list->VtxBuffer.size();
		}
	}

	void d3d11_runtime::draw_debug_menu()
	{
		ImGui::Text("MSAA is %s", _is_multisampling_enabled ? "active" : "inactive");
		ImGui::Spacing();

#if RESHADE_DX11_CAPTURE_DEPTH_BUFFERS
		if (ImGui::CollapsingHeader("Depth and Intermediate Buffers", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool modified = false;
			modified |= ImGui::Combo("Depth Texture Format", &depth_buffer_texture_format, "All\0D16\0D32F\0D24S8\0D32FS8\0");

			if (modified)
			{
				runtime::save_config();
				_current_tracker.reset();
				create_depthstencil_replacement(nullptr, nullptr);
				return;
			}

			modified |= ImGui::Checkbox("Copy depth before clearing", &depth_buffer_before_clear);

			if (depth_buffer_before_clear)
			{
				if (ImGui::Checkbox("Extended depth buffer detection", &extended_depth_buffer_detection))
				{
					cleared_depth_buffer_index = 0;
					modified = true;
				}

				_current_tracker.keep_cleared_depth_textures();

				ImGui::Spacing();
				ImGui::TextUnformatted("Depth Buffers:");

				UINT current_index = 1;

				for (const auto &it : _current_tracker.cleared_depth_textures())
				{
					char label[512] = "";
					sprintf_s(label, "%s%2u", (current_index == cleared_depth_buffer_index ? "> " : "  "), current_index);

					if (bool value = cleared_depth_buffer_index == current_index; ImGui::Checkbox(label, &value))
					{
						cleared_depth_buffer_index = value ? current_index : 0;
						modified = true;
					}

					ImGui::SameLine();

					ImGui::Text("=> 0x%p | 0x%p | %ux%u", it.second.src_depthstencil.get(), it.second.src_texture.get(), it.second.src_texture_desc.Width, it.second.src_texture_desc.Height);

					if (it.second.dest_texture != nullptr)
					{
						ImGui::SameLine();

						ImGui::Text("=> %p", it.second.dest_texture.get());
					}

					current_index++;
				}
			}
			else if (!_current_tracker.depth_buffer_counters().empty())
			{
				ImGui::Spacing();
				ImGui::TextUnformatted("Depth Buffers: (intermediate buffer draw calls in parentheses)");

				for (const auto &[depthstencil, snapshot] : _current_tracker.depth_buffer_counters())
				{
					char label[512] = "";
					sprintf_s(label, "%s0x%p", (depthstencil == _depthstencil ? "> " : "  "), depthstencil.get());

					if (bool value = _best_depth_stencil_overwrite == depthstencil; ImGui::Checkbox(label, &value))
					{
						_best_depth_stencil_overwrite = value ? depthstencil.get() : nullptr;

						com_ptr<ID3D11Texture2D> texture = snapshot.texture;

						if (texture == nullptr && _best_depth_stencil_overwrite != nullptr)
						{
							com_ptr<ID3D11Resource> resource;
							_best_depth_stencil_overwrite->GetResource(&resource);

							resource->QueryInterface(&texture);
						}

						create_depthstencil_replacement(_best_depth_stencil_overwrite, texture.get());
					}

					ImGui::SameLine();

					std::string additional_view_label;

					if (!snapshot.additional_views.empty())
					{
						additional_view_label += '(';

						for (auto const &[view, stats] : snapshot.additional_views)
							additional_view_label += std::to_string(stats.drawcalls) + ", ";

						// Remove last ", " from string
						additional_view_label.pop_back();
						additional_view_label.pop_back();

						additional_view_label += ')';
					}

					ImGui::Text("| %5u draw calls ==> %8u vertices, %2u additional render target%c %s", snapshot.stats.drawcalls, snapshot.stats.vertices, snapshot.additional_views.size(), snapshot.additional_views.size() != 1 ? 's' : ' ', additional_view_label.c_str());
				}
			}

			if (modified)
			{
				runtime::save_config();
			}
		}
#endif
#if RESHADE_DX11_CAPTURE_CONSTANT_BUFFERS
		if (ImGui::CollapsingHeader("Constant Buffers", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const auto &[buffer, counter] : _current_tracker.constant_buffer_counters())
			{
				bool likely_camera_transform_buffer = false;

				D3D11_BUFFER_DESC desc;
				buffer->GetDesc(&desc);

				if (counter.ps_uses > 0 && counter.vs_uses > 0 && counter.mapped < .10 * counter.vs_uses && desc.ByteWidth > 1000)
					likely_camera_transform_buffer = true;

				ImGui::Text("%s 0x%p | used in %4u vertex shaders and %4u pixel shaders, mapped %3u times, %8u bytes", likely_camera_transform_buffer ? ">" : " ", buffer.get(), counter.vs_uses, counter.ps_uses, counter.mapped, desc.ByteWidth);
			}
		}
#endif
	}

#if RESHADE_DX11_CAPTURE_DEPTH_BUFFERS
	void d3d11_runtime::detect_depth_source(draw_call_tracker &tracker)
	{
		if (depth_buffer_before_clear)
		{
			_best_depth_stencil_overwrite = nullptr;
		}

		if (_best_depth_stencil_overwrite != nullptr)
		{
			return;
		}

		static int cooldown = 0, traffic = 0;

		if (cooldown-- > 0)
		{
			traffic += g_network_traffic > 0;

			if (!depth_buffer_before_clear)
			{
				return;
			}
		}
		else
		{
			cooldown = 30;
			if (traffic > 10)
			{
				traffic = 0;
				create_depthstencil_replacement(nullptr, nullptr);
				return;
			}
			else
			{
				traffic = 0;
			}
		}

		if (_is_multisampling_enabled)
		{
			return;
		}

		if (depth_buffer_before_clear)
		{
			// At the final rendering stage, it is fine to rely on the depth stencil to select the best depth texture
			// But when we retrieve the depth textures before the final rendering stage, there is chance that one or many different depth textures are associated to the same depth stencil (for instance, in Bioshock 2)
			// In this case, we cannot use the depth stencil to determine which depth texture is the good one, so we can use the default depth stencil
			// For the moment, the best we can do is retrieve all the depth textures that has been cleared in the rendering pipeline, then select one of them (by default, the last one)
			// In the future, maybe we could find a way to retrieve depth texture statistics (number of draw calls and number of vertices), so ReShade could automatically select the best one
			ID3D11Texture2D *const best_match_texture = tracker.find_best_cleared_depth_buffer_texture(cleared_depth_buffer_index);

			if (best_match_texture != nullptr)
			{
				create_depthstencil_replacement(_default_depthstencil.get(), best_match_texture);
			}
			return;
		}

		const auto best_snapshot = tracker.find_best_snapshot(_width, _height);

		if (best_snapshot.depthstencil != nullptr)
		{
			create_depthstencil_replacement(best_snapshot.depthstencil, best_snapshot.texture.get());
		}
	}

	bool d3d11_runtime::create_depthstencil_replacement(ID3D11DepthStencilView *depthstencil, ID3D11Texture2D *texture)
	{
		_depthstencil.reset();
		_depthstencil_replacement.reset();
		_depthstencil_texture.reset();
		_depthstencil_texture_srv.reset();

		if (depthstencil != nullptr)
		{
			assert(texture != nullptr);

			_depthstencil = depthstencil;
			_depthstencil_texture = texture;

			D3D11_TEXTURE2D_DESC texdesc;
			_depthstencil_texture->GetDesc(&texdesc);

			HRESULT hr = S_OK;

			if ((texdesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
			{
				_depthstencil_texture.reset();

				switch (texdesc.Format)
				{
					case DXGI_FORMAT_R16_TYPELESS:
					case DXGI_FORMAT_D16_UNORM:
						texdesc.Format = DXGI_FORMAT_R16_TYPELESS;
						break;
					case DXGI_FORMAT_R32_TYPELESS:
					case DXGI_FORMAT_D32_FLOAT:
						texdesc.Format = DXGI_FORMAT_R32_TYPELESS;
						break;
					default:
					case DXGI_FORMAT_R24G8_TYPELESS:
					case DXGI_FORMAT_D24_UNORM_S8_UINT:
						texdesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
						break;
					case DXGI_FORMAT_R32G8X24_TYPELESS:
					case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
						texdesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
						break;
				}

				texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

				hr = _device->CreateTexture2D(&texdesc, nullptr, &_depthstencil_texture);

				if (SUCCEEDED(hr))
				{
					D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc = { };
					dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

					switch (texdesc.Format)
					{
						case DXGI_FORMAT_R16_TYPELESS:
							dsvdesc.Format = DXGI_FORMAT_D16_UNORM;
							break;
						case DXGI_FORMAT_R32_TYPELESS:
							dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
							break;
						case DXGI_FORMAT_R24G8_TYPELESS:
							dsvdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
							break;
						case DXGI_FORMAT_R32G8X24_TYPELESS:
							dsvdesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
							break;
					}

					hr = _device->CreateDepthStencilView(_depthstencil_texture.get(), &dsvdesc, &_depthstencil_replacement);
				}
			}

			if (FAILED(hr))
			{
				LOG(ERROR) << "Failed to create depth stencil replacement texture! HRESULT is '" << std::hex << hr << std::dec << "'.";

				return false;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc = { };
			srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvdesc.Texture2D.MipLevels = 1;

			switch (texdesc.Format)
			{
				case DXGI_FORMAT_R16_TYPELESS:
					srvdesc.Format = DXGI_FORMAT_R16_FLOAT;
					break;
				case DXGI_FORMAT_R32_TYPELESS:
					srvdesc.Format = DXGI_FORMAT_R32_FLOAT;
					break;
				case DXGI_FORMAT_R24G8_TYPELESS:
					srvdesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				case DXGI_FORMAT_R32G8X24_TYPELESS:
					srvdesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
					break;
			}

			hr = _device->CreateShaderResourceView(_depthstencil_texture.get(), &srvdesc, &_depthstencil_texture_srv);

			if (FAILED(hr))
			{
				LOG(ERROR) << "Failed to create depth stencil replacement resource view! HRESULT is '" << std::hex << hr << std::dec << "'.";

				return false;
			}

			// Update auto depth stencil
			com_ptr<ID3D11DepthStencilView> current_depthstencil;
			ID3D11RenderTargetView *targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

			_immediate_context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, &current_depthstencil);

			if (current_depthstencil != nullptr && current_depthstencil == _depthstencil)
			{
				_immediate_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, _depthstencil_replacement.get());
			}

			for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			{
				if (targets[i] != nullptr)
				{
					targets[i]->Release();
				}
			}
		}

		// Update effect textures
		_effect_shader_resources[2] = _depthstencil_texture_srv;
		for (const auto &technique : _techniques)
			for (const auto &pass : technique.passes)
				pass->as<d3d11_pass_data>()->shader_resources[2] = _depthstencil_texture_srv;

		return true;
	}

	com_ptr<ID3D11Texture2D> d3d11_runtime::select_depth_texture_save(D3D11_TEXTURE2D_DESC &texture_desc)
	{
		/* function that selects the appropriate texture where we want to save the depth texture before it is cleared  */
		/* if this texture is null, create it according to the dimensions and the format of the depth texture */
		/* Doing so, we avoid to create a new texture each time the depth texture is saved */

		// select the texture format according to the depth texture's one
		switch (texture_desc.Format)
		{
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_D16_UNORM:
			texture_desc.Format = DXGI_FORMAT_R16_TYPELESS;
			break;
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
			texture_desc.Format = DXGI_FORMAT_R32_TYPELESS;
			break;
		default:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			texture_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
			break;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			texture_desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
			break;
		}

		// create an unique index based on the dept texture format and dimensions
		UINT idx = texture_desc.Format * texture_desc.Width * texture_desc.Height;

		const auto it = _depth_texture_saves.find(idx);
		com_ptr<ID3D11Texture2D> depth_texture_save = nullptr;

		// Create the saved texture pointed by the index if it does not already exist
		if (it == _depth_texture_saves.end())
		{
			texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

			HRESULT hr = _device->CreateTexture2D(&texture_desc, nullptr, &depth_texture_save);

			if (FAILED(hr))
			{
				LOG(ERROR) << "Failed to create depth texture copy! HRESULT is '" << std::hex << hr << std::dec << "'.";
				return nullptr;
			}

			_depth_texture_saves.emplace(idx, depth_texture_save);
		}
		// If the saved texture pointed by the index exists, use it (save resources and perfs)
		else
		{
			depth_texture_save = it->second;
		}

		return depth_texture_save;
	}
#endif
}
