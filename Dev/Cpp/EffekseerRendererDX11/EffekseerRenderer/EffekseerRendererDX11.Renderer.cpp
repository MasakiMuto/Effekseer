
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerRendererDX11.Renderer.h"
#include "EffekseerRendererDX11.RendererImplemented.h"
#include "EffekseerRendererDX11.RenderState.h"

#include "EffekseerRendererDX11.Shader.h"
#include "EffekseerRendererDX11.VertexBuffer.h"
#include "EffekseerRendererDX11.IndexBuffer.h"
#include "EffekseerRendererDX11.DeviceObject.h"
#include "EffekseerRendererDX11.SpriteRenderer.h"
#include "EffekseerRendererDX11.RibbonRenderer.h"
#include "EffekseerRendererDX11.RingRenderer.h"
#include "EffekseerRendererDX11.ModelRenderer.h"
#include "EffekseerRendererDX11.TrackRenderer.h"
#include "EffekseerRendererDX11.TextureLoader.h"
#include "EffekseerRendererDX11.ModelLoader.h"

#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
#include "../../EffekseerRendererCommon/EffekseerRenderer.PngTextureLoader.h"
#endif

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace EffekseerRendererDX11
{
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
OriginalState::OriginalState()
	: m_blendState		( NULL )
	, m_depthStencilState	( NULL )
	, m_depthStencilStateRef	( 0 )
	, m_vertexConstantBuffer	( NULL )
	, m_pixelConstantBuffer		( NULL )
	, m_layout					( NULL )
{
	for( int32_t i = 0; i < 4; i++ )
	{
		m_samplers[i] = NULL;
	}

	for (int32_t i = 0; i < 4; i++)
	{
		m_psSRVs[i] = nullptr;
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
OriginalState::~OriginalState()
{
	ReleaseState();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::SaveState(ID3D11Device* device, ID3D11DeviceContext* context )
{
	context->PSGetSamplers( 0, 4, m_samplers );
	context->OMGetBlendState( &m_blendState, m_blendFactor, &m_blendSampleMask );
	context->OMGetDepthStencilState( &m_depthStencilState, &m_depthStencilStateRef );
	context->RSGetState(&m_pRasterizerState);

	context->VSGetConstantBuffers(0, 1, &m_vertexConstantBuffer);
	context->PSGetConstantBuffers(0, 1, &m_pixelConstantBuffer);

	context->VSGetShader(&m_pVS, nullptr, nullptr);
	context->PSGetShader(&m_pPS, nullptr, nullptr);

	context->IAGetInputLayout( &m_layout );

	context->IAGetPrimitiveTopology(&m_topology);

	context->PSGetShaderResources(0, 4, m_psSRVs);

	context->IAGetVertexBuffers(0, 1, &m_pVB, &m_vbStrides, &m_vbOffset);
	context->IAGetIndexBuffer(&m_pIB, &m_ibFormat, &m_ibOffset);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::LoadState(ID3D11Device* device, ID3D11DeviceContext* context )
{
	context->PSSetSamplers(0, 4, m_samplers );
	context->OMSetBlendState( m_blendState, m_blendFactor, m_blendSampleMask );
	context->OMSetDepthStencilState( m_depthStencilState, m_depthStencilStateRef );
	context->RSSetState(m_pRasterizerState);

	context->VSSetConstantBuffers(0, 1, &m_vertexConstantBuffer);
	context->PSSetConstantBuffers(0, 1, &m_pixelConstantBuffer);

	context->VSSetShader(m_pVS, NULL, 0);
	context->PSSetShader(m_pPS, NULL, 0);

	context->IASetInputLayout( m_layout );

	context->IASetPrimitiveTopology(m_topology);

	context->PSSetShaderResources(0, 4, m_psSRVs);

	context->IASetVertexBuffers(0, 1, &m_pVB, &m_vbStrides, &m_vbOffset);
	context->IASetIndexBuffer(m_pIB, m_ibFormat, m_ibOffset);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::ReleaseState()
{	
	for( int32_t i = 0; i < 4; i++ )
	{
		ES_SAFE_RELEASE( m_samplers[i] );
	}
	ES_SAFE_RELEASE( m_blendState );

	ES_SAFE_RELEASE( m_depthStencilState );

	ES_SAFE_RELEASE(m_pRasterizerState);

	ES_SAFE_RELEASE( m_vertexConstantBuffer );
	ES_SAFE_RELEASE( m_pixelConstantBuffer );

	ES_SAFE_RELEASE(m_pVS);
	ES_SAFE_RELEASE(m_pPS);

	ES_SAFE_RELEASE( m_layout );

	for (int32_t i = 0; i < 4; i++)
	{
		ES_SAFE_RELEASE(m_psSRVs[i]);
	}

	ES_SAFE_RELEASE(m_pVB);
	ES_SAFE_RELEASE(m_pIB);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Renderer* Renderer::Create( ID3D11Device* device, ID3D11DeviceContext* context, int32_t squareMaxCount )
{
	RendererImplemented* renderer = new RendererImplemented( squareMaxCount );
	if( renderer->Initialize( device, context ) )
	{
		return renderer;
	}
	return NULL;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::RendererImplemented( int32_t squareMaxCount )
	: m_reference	( 1 )
	, m_device	( NULL )
	, m_context	( NULL )
	, m_vertexBuffer( NULL )
	, m_indexBuffer	( NULL )
	, m_squareMaxCount	( squareMaxCount )
	, m_coordinateSystem	( ::Effekseer::CoordinateSystem::RH )
	, m_renderState		( NULL )
	, m_restorationOfStates( true )
{
	SetLightDirection( ::Effekseer::Vector3D( 1.0f, 1.0f, 1.0f ) );
	SetLightColor( ::Effekseer::Color( 255, 255, 255, 255 ) );
	SetLightAmbientColor( ::Effekseer::Color( 0, 0, 0, 0 ) );

	m_state = new OriginalState();

#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	EffekseerRenderer::PngTextureLoader::Initialize();
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::~RendererImplemented()
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	EffekseerRenderer::PngTextureLoader::Finalize();
#endif

	assert( m_reference == 0 );

	ES_SAFE_DELETE( m_state );

	ES_SAFE_DELETE( m_renderState );
	ES_SAFE_DELETE( m_vertexBuffer );
	ES_SAFE_DELETE( m_indexBuffer );
	assert( m_reference == -2 );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::OnLostDevice()
{
	for (auto& device : m_deviceObjects)
	{
		device->OnLostDevice();
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::OnResetDevice()
{
	for (auto& device : m_deviceObjects)
	{
		device->OnResetDevice();
	}
}


//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::Initialize( ID3D11Device* device, ID3D11DeviceContext* context )
{
	m_device = device;
	m_context = context;

	// 頂点の生成
	{
		// 最大でfloat * 10 と仮定
		m_vertexBuffer = VertexBuffer::Create( this, sizeof(float) * 10 * m_squareMaxCount * 4, true );
		if( m_vertexBuffer == NULL ) return false;
	}

	// インデックスの生成
	{
		m_indexBuffer = IndexBuffer::Create( this, m_squareMaxCount * 6, false );
		if( m_indexBuffer == NULL ) return false;

		m_indexBuffer->Lock();

		// ( 標準設定で　DirectX 時計周りが表, OpenGLは反時計回りが表 )
		for( int i = 0; i < m_squareMaxCount; i++ )
		{
			uint16_t* buf = (uint16_t*)m_indexBuffer->GetBufferDirect( 6 );
			buf[0] = 3 + 4 * i;
			buf[1] = 1 + 4 * i;
			buf[2] = 0 + 4 * i;
			buf[3] = 3 + 4 * i;
			buf[4] = 0 + 4 * i;
			buf[5] = 2 + 4 * i;
		}

		m_indexBuffer->Unlock();
	}

	m_renderState = new RenderState( this );


	// 参照カウントの調整
	// m_vertexBufferの参照カウンタ
	Release();
	// m_indexBufferの参照カウンタ
	Release();

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int RendererImplemented::AddRef()
{
	m_reference++;
	return m_reference;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int RendererImplemented::Release()
{
	m_reference--;
	int count = m_reference;
	if ( count == 0 )
	{
		delete this;
	}
	return count;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::Destory()
{
	Release();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetRestorationOfStatesFlag(bool flag)
{
	m_restorationOfStates = flag;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::BeginRendering()
{
	assert( m_device != NULL );

	::Effekseer::Matrix44::Mul( m_cameraProj, m_camera, m_proj );
	
	// ステートを保存する
	if( m_restorationOfStates )
	{
		m_state->SaveState( m_device, m_context );
	}

	// ステート初期設定
	m_renderState->GetActiveState().Reset();
	m_renderState->Update( true );

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::EndRendering()
{
	assert( m_device != NULL );
	
	// ステートを復元する
	if( m_restorationOfStates )
	{
		m_state->LoadState( m_device, m_context );
		m_state->ReleaseState();
	}

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ID3D11Device* RendererImplemented::GetDevice()
{
	return m_device;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ID3D11DeviceContext* RendererImplemented::GetContext()
{
	return m_context;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
VertexBuffer* RendererImplemented::GetVertexBuffer()
{
	return m_vertexBuffer;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
IndexBuffer* RendererImplemented::GetIndexBuffer()
{
	return m_indexBuffer;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int32_t RendererImplemented::GetSquareMaxCount() const
{
	return m_squareMaxCount;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::EffekseerRenderer::RenderStateBase* RendererImplemented::GetRenderState()
{
	return m_renderState;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Vector3D& RendererImplemented::GetLightDirection() const
{
	return m_lightDirection;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightDirection( ::Effekseer::Vector3D& direction )
{
	m_lightDirection = direction;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Color& RendererImplemented::GetLightColor() const
{
	return m_lightColor;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightColor( ::Effekseer::Color& color )
{
	m_lightColor = color;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Color& RendererImplemented::GetLightAmbientColor() const
{
	return m_lightAmbient;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightAmbientColor( ::Effekseer::Color& color )
{
	m_lightAmbient = color;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Matrix44& RendererImplemented::GetProjectionMatrix() const
{
	return m_proj;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetProjectionMatrix( const ::Effekseer::Matrix44& mat )
{
	m_proj = mat;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Matrix44& RendererImplemented::GetCameraMatrix() const
{
	return m_camera;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetCameraMatrix( const ::Effekseer::Matrix44& mat )
{
	m_camera = mat;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::Matrix44& RendererImplemented::GetCameraProjectionMatrix()
{
	return m_cameraProj;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::SpriteRenderer* RendererImplemented::CreateSpriteRenderer()
{
	return SpriteRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RibbonRenderer* RendererImplemented::CreateRibbonRenderer()
{
	return RibbonRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RingRenderer* RendererImplemented::CreateRingRenderer()
{
	return RingRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelRenderer* RendererImplemented::CreateModelRenderer()
{
	return ModelRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TrackRenderer* RendererImplemented::CreateTrackRenderer()
{
	return TrackRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TextureLoader* RendererImplemented::CreateTextureLoader( ::Effekseer::FileInterface* fileInterface )
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new TextureLoader( this, fileInterface );
#else
	return NULL;
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelLoader* RendererImplemented::CreateModelLoader( ::Effekseer::FileInterface* fileInterface )
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new ModelLoader( this, fileInterface );
#else
	return NULL;
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetVertexBuffer(ID3D11Buffer* vertexBuffer, int32_t size)
{
	ID3D11Buffer* vBuf = vertexBuffer;
	uint32_t vertexSize = size;
	uint32_t offset = 0;
	GetContext()->IASetVertexBuffers( 0, 1, &vBuf, &vertexSize, &offset ); 
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetVertexBuffer( VertexBuffer* vertexBuffer, int32_t size )
{
	ID3D11Buffer* vBuf = vertexBuffer->GetInterface();
	uint32_t vertexSize = size;
	uint32_t offset = 0;
	GetContext()->IASetVertexBuffers( 0, 1, &vBuf, &vertexSize, &offset ); 
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetIndexBuffer( IndexBuffer* indexBuffer )
{
	GetContext()->IASetIndexBuffer( indexBuffer->GetInterface(), DXGI_FORMAT_R16_UINT, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetIndexBuffer(ID3D11Buffer* indexBuffer)
{
	GetContext()->IASetIndexBuffer( indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLayout( Shader* shader )
{
	GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	GetContext()->IASetInputLayout( shader->GetLayoutInterface() );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawSprites( int32_t spriteCount, int32_t vertexOffset )
{
	GetContext()->DrawIndexed(
		spriteCount * 2 * 3,
		0,
		vertexOffset );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawPolygon( int32_t vertexCount, int32_t indexCount)
{
	GetContext()->DrawIndexed(
		indexCount,
		0,
		0 );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::BeginShader(Shader* shader)
{
	GetContext()->VSSetShader( shader->GetVertexShader(), NULL, 0);
	GetContext()->PSSetShader( shader->GetPixelShader(), NULL, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::EndShader(Shader* shader)
{

}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetTextures(Shader* shader, ID3D11ShaderResourceView** textures, int32_t count)
{
	GetContext()->PSSetShaderResources(0, count, textures);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::ResetRenderState()
{
	m_renderState->GetActiveState().Reset();
	m_renderState->Update( true );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
}
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
