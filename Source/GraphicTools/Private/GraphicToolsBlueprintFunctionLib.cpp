#include "GraphicToolsBlueprintFunctionLib.h"

#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterStruct.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"
#include "Runtime/RenderCore/Public/RenderTargetPool.h"


#define LOCTEXT_NAMESPACE "GraphicToolsPlugin"



class FCheckerBoardComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCheckerBoardComputeShader, Global)

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCheckerBoardComputeShader() {}
	FCheckerBoardComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		FTexture2DRHIRef &InOutputSurfaceValue,
		FUnorderedAccessViewRHIRef& UAV
	)
	{
		FRHIComputeShader* ShaderRHI = RHICmdList.GetBoundComputeShader();

		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, UAV);
		OutputSurface.SetTexture(RHICmdList, ShaderRHI, InOutputSurfaceValue, UAV);
	}

	void UnsetParameters(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef& UAV)
	{
		RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, UAV);
		OutputSurface.UnsetUAV(RHICmdList, RHICmdList.GetBoundComputeShader());
	}

private:

	LAYOUT_FIELD(FRWShaderParameter, OutputSurface);
};



class FMyShaderVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyShaderVS, Global);
public:
	FMyShaderVS() {}
	FMyShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		
	}

};

class FMyShaderPS : public FGlobalShader
{
	//DECLARE_SHADER_TYPE(FMyShaderPS, Global);
public:
	DECLARE_GLOBAL_SHADER(FMyShaderPS);
	SHADER_USE_PARAMETER_STRUCT_WITH_LEGACY_BASE(FMyShaderPS, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER(FVector4, SimpleColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, Texture1)
	SHADER_PARAMETER_SAMPLER(SamplerState, TextureSample1)
	END_SHADER_PARAMETER_STRUCT()
	
};
//IMPLEMENT_SHADER_TYPE(, FMyShaderVS, TEXT("/Plugin/GraphicTools/Private/CheckerBoard.usf"), TEXT("MainVS"), SF_Vertex);
//IMPLEMENT_SHADER_TYPE(, FMyShaderPS, TEXT("/Plugin/GraphicTools/Private/CheckerBoard.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FMyShaderVS, "/Plugin/GraphicTools/Private/DrawColorShader.usf", "MainVS", SF_Vertex);  
IMPLEMENT_GLOBAL_SHADER(FMyShaderPS, "/Plugin/GraphicTools/Private/DrawColorShader.usf", "MainPS", SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FCheckerBoardComputeShader, TEXT("/Plugin/GraphicTools/Private/CheckerBoard.usf"), TEXT("MainCS"), SF_Compute);
struct FTextureVertex
{
public:
	FVector4 Position;
	FVector2D UV;
};
static void DrawColorBoard_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* TextureRenderTargetResource,
	FLinearColor Color,
	UTexture2D* Tex,
	ERHIFeatureLevel::Type FeatureLevel
	)
{
	FRHITexture2D* RenderTargetTexture = TextureRenderTargetResource->GetRenderTargetTexture();
	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store, TextureRenderTargetResource->TextureRHI);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("shaderTest"));
	{

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);  
		TShaderMapRef<FMyShaderVS> VertexShader(GlobalShaderMap);  
		TShaderMapRef<FMyShaderPS> PixelShader(GlobalShaderMap);
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		FVertexDeclarationElementList Elements;
		const auto Stride = sizeof(FTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
		FVertexDeclarationRHIRef VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
		
		
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		FMyShaderPS::FParameters PsParameter;
		
		PsParameter.SimpleColor = Color;
		PsParameter.TextureSample1 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PsParameter.Texture1 = Tex->GetResource()->GetTexture2DRHI();
		SetShaderParameters(RHICmdList,PixelShader,PixelShader.GetPixelShader(),PsParameter);
		//SetShaderValue(RHICmdList, PixelShader.GetPixelShader(), PixelShader->SimpleColor,Color);

		FRHIResourceCreateInfo CreateInfo;
		FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(sizeof(FTextureVertex) * 4, BUF_Volatile, CreateInfo);
		void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FTextureVertex) * 4, RLM_WriteOnly);

		FTextureVertex* Vertices = (FTextureVertex*)VoidPtr;
		Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
		Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
		Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
		Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
		Vertices[0].UV = FVector2D(0, 0);
		Vertices[1].UV = FVector2D(1, 0);
		Vertices[2].UV = FVector2D(0, 1);
		Vertices[3].UV = FVector2D(1, 1);
		RHIUnlockVertexBuffer(VertexBufferRHI);

		RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
		RHICmdList.DrawPrimitive(0, 2, 1);
	}
	RHICmdList.EndRenderPass();
}

static void DrawCheckerBoard_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* TextureRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel
)
{
	check(IsInRenderingThread());

	FTexture2DRHIRef RenderTargetTexture = TextureRenderTargetResource->GetRenderTargetTexture();
	uint32 GGroupSize = 32;
	FIntPoint FullResolution = FIntPoint(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY());
	uint32 GroupSizeX = FMath::DivideAndRoundUp((uint32)RenderTargetTexture->GetSizeX(), GGroupSize);
	uint32 GroupSizeY = FMath::DivideAndRoundUp((uint32)RenderTargetTexture->GetSizeY(), GGroupSize);

	TShaderMapRef<FCheckerBoardComputeShader>ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	FRHIResourceCreateInfo CreateInfo;
	//Create a temp resource
	FTexture2DRHIRef GSurfaceTexture2D = RHICreateTexture2D(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY(), PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef GUAV = RHICreateUnorderedAccessView(GSurfaceTexture2D);

	ComputeShader->SetParameters(RHICmdList, RenderTargetTexture, GUAV);
	DispatchComputeShader(RHICmdList, ComputeShader, GroupSizeX, GroupSizeY, 1);
	ComputeShader->UnsetParameters(RHICmdList, GUAV);

	FRHICopyTextureInfo CopyInfo;
	RHICmdList.CopyTexture(GSurfaceTexture2D, RenderTargetTexture, CopyInfo);
}
void UGraphicToolsBlueprintFunctionLib::DrawCheckerBoard(const UObject* WorldContextObject,
	UTextureRenderTarget2D* OutputRenderTarget)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		FMessageLog("Blueprint").Warning(
			LOCTEXT("UGraphicToolsBlueprintLibrary::DrawCheckerBoard",
				"DrawUVDisplacementToRenderTarget: Output render target is required."));
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	(
		[TextureRenderTargetResource, FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{
			DrawCheckerBoard_RenderThread
			(
				RHICmdList,
				TextureRenderTargetResource,
				FeatureLevel
			);
		}
	);
}

void UGraphicToolsBlueprintFunctionLib::DrawColorBoard(const UObject* WorldContextObject,
	UTextureRenderTarget2D* OutputRenderTarget,
	UTexture2D* Tex,
	FLinearColor Color)
{
	check(IsInGameThread());

	if (!OutputRenderTarget || !Tex)
	{
		FMessageLog("Blueprint").Warning(
			LOCTEXT("UGraphicToolsBlueprintLibrary::DrawCheckerBoard",
				"DrawUVDisplacementToRenderTarget: Output render target is required."));
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	(
		[TextureRenderTargetResource, Color,Tex, FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{
			DrawColorBoard_RenderThread
			(
				RHICmdList,
				TextureRenderTargetResource,
				Color,
				Tex,
				FeatureLevel
			);
		}
	);
}


#undef LOCTEXT_NAMESPACE
