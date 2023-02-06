#pragma once
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GraphicToolsBlueprintFunctionLib.generated.h"
 
UCLASS(MinimalAPI, meta=(ScriptName = "GraphicTools"))
class UGraphicToolsBlueprintFunctionLib :  public  UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UGraphicToolsBlueprintFunctionLib(){}
	UFUNCTION(BlueprintCallable, Category = "SLSGraphicTools", meta = (WorldContext = "WorldContextObject"))
	static void DrawCheckerBoard(
		const UObject* WorldContextObject,
		class UTextureRenderTarget2D* OutputRenderTarget);
	UFUNCTION(BlueprintCallable, Category = "SLSGraphicTools", meta = (WorldContext = "WorldContextObject"))
	static void DrawColorBoard(
		const UObject* WorldContextObject,
		class UTextureRenderTarget2D* OutputRenderTarget,
		UTexture2D* Tex,
		FLinearColor Color);
};
