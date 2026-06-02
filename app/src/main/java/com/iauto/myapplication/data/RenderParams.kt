package com.iauto.myapplication.data

import android.os.Parcel
import android.os.Parcelable

/**
 * 相机参数
 * 默认值与 Scene.cpp 中的默认配置保持一致
 * - 从车侧后方斜上方俯视，距车约 7m
 * - 相机高度 2m，看向车身中部 (Y=0.5)
 */
data class CameraParams(
    var position: Vec3 = Vec3(0f, 2f, 7f),     // 车后方 7m，高度 2m
    var target: Vec3 = Vec3(0f, 0.5f, 0f),     // 看向车身中部
    var fov: Float = 45f,                       // 45 度 FOV
    var nearPlane: Float = 0.1f,
    var farPlane: Float = 100f,
    var orthographic: Boolean = false,
    var orthoSize: Float = 10f
) : Parcelable {
    constructor(parcel: Parcel) : this(
        position = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        target = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        fov = parcel.readFloat(),
        nearPlane = parcel.readFloat(),
        farPlane = parcel.readFloat(),
        orthographic = parcel.readBoolean(),
        orthoSize = parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeParcelable(position, flags)
        parcel.writeParcelable(target, flags)
        parcel.writeFloat(fov)
        parcel.writeFloat(nearPlane)
        parcel.writeFloat(farPlane)
        parcel.writeBoolean(orthographic)
        parcel.writeFloat(orthoSize)
    }

    override fun describeContents(): Int = 0

    companion object CREATOR : Parcelable.Creator<CameraParams> {
        override fun createFromParcel(parcel: Parcel): CameraParams = CameraParams(parcel)
        override fun newArray(size: Int): Array<CameraParams?> = arrayOfNulls(size)
    }
}

/**
 * 场景配置
 */
data class SceneConfig(
    var bgColor: Vec3 = Vec3(0.1f, 0.1f, 0.15f),
    var ambientIntensity: Float = 1f,
    // Shadows
    var shadowEnabled: Boolean = false,  // 默认关闭，需要手动开启
    var lightPosX: Float = 10f,          // 光源 X 位置 - 增加以改善阴影角度
    var lightPosY: Float = 15f,          // 光源 Y 位置 - 更高以获得更好的阴影覆盖
    var lightPosZ: Float = 10f,          // 光源 Z 位置
    var shadowSoftness: Float = 0.5f,
    var shadowBias: Float = 0.05f,       // 增加以改善阴影可见性
    var shadowMapSize: Int = 2048,
    var shadowPCFEnabled: Boolean = true,  // PCF 软阴影开关
    // SSAO
    var ssaoEnabled: Boolean = false,
    var ssaoRadius: Float = 0.5f,
    var ssaoIntensity: Float = 1f,
    // Planar Reflection (地面倒影)
    var reflectionEnabled: Boolean = false,  // 默认关闭，需要手动开启
    var reflectionPlaneNormal: Vec3 = Vec3(0f, 1f, 0f),  // 地面法线 (+Y)
    var reflectionPlaneDistance: Float = 0f,  // 平面到原点距离
    var reflectionResolution: Int = 512,  // 256/512/1024/2048
    var reflectionFadeDistance: Float = 10f,  // 淡出距离
    var reflectionBlur: Float = 0.1f  // 模糊程度（模拟粗糙地面）
) : Parcelable {
    constructor(parcel: Parcel) : this(
        bgColor = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        ambientIntensity = parcel.readFloat(),
        shadowEnabled = parcel.readBoolean(),
        lightPosX = parcel.readFloat(),
        lightPosY = parcel.readFloat(),
        lightPosZ = parcel.readFloat(),
        shadowSoftness = parcel.readFloat(),
        shadowBias = parcel.readFloat(),
        shadowMapSize = parcel.readInt(),
        shadowPCFEnabled = parcel.readBoolean(),
        ssaoEnabled = parcel.readBoolean(),
        ssaoRadius = parcel.readFloat(),
        ssaoIntensity = parcel.readFloat(),
        reflectionEnabled = parcel.readBoolean(),
        reflectionPlaneNormal = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        reflectionPlaneDistance = parcel.readFloat(),
        reflectionResolution = parcel.readInt(),
        reflectionFadeDistance = parcel.readFloat(),
        reflectionBlur = parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeParcelable(bgColor, flags)
        parcel.writeFloat(ambientIntensity)
        parcel.writeBoolean(shadowEnabled)
        parcel.writeFloat(lightPosX)
        parcel.writeFloat(lightPosY)
        parcel.writeFloat(lightPosZ)
        parcel.writeFloat(shadowSoftness)
        parcel.writeFloat(shadowBias)
        parcel.writeInt(shadowMapSize)
        parcel.writeBoolean(shadowPCFEnabled)
        parcel.writeBoolean(ssaoEnabled)
        parcel.writeFloat(ssaoRadius)
        parcel.writeFloat(ssaoIntensity)
        parcel.writeBoolean(reflectionEnabled)
        parcel.writeParcelable(reflectionPlaneNormal, flags)
        parcel.writeFloat(reflectionPlaneDistance)
        parcel.writeInt(reflectionResolution)
        parcel.writeFloat(reflectionFadeDistance)
        parcel.writeFloat(reflectionBlur)
    }

    override fun describeContents(): Int = 0

    companion object CREATOR : Parcelable.Creator<SceneConfig> {
        override fun createFromParcel(parcel: Parcel): SceneConfig = SceneConfig(parcel)
        override fun newArray(size: Int): Array<SceneConfig?> = arrayOfNulls(size)
    }
}

/**
 * 后处理配置
 * 默认值与 Scene.cpp 中的默认配置保持一致
 */
data class PostProcessConfig(
    var aaMode: Int = 0,               // 0: TAA(default), 1: FXAA, 2: None
    var bloomEnabled: Boolean = false, // Scene 默认：false
    var bloomIntensity: Float = 1.5f,  // Scene 默认：1.5f
    var bloomThreshold: Float = 0.8f,  // Scene 默认：0.8f
    var tonemapEnabled: Boolean = true, // Scene 默认：true
    var tonemapOp: Int = 1,             // Scene 默认：1 (ACES)
    var exposure: Float = 0.4f,         // Scene 默认：0.4f (HDR cubemap 亮度偏高，压低)
    var gammaCorrection: Float = 2.2f,
    var vignetteEnabled: Boolean = false,
    var vignetteIntensity: Float = 0.5f,
    // Fog 参数
    var fogEnabled: Boolean = false,
    var fogMode: Int = 1,                // 0: linear, 1: exp(default), 2: exp2
    var fogDensity: Float = 0.25f,
    var fogStart: Float = 1.0f,
    var fogEnd: Float = 8.0f,
    var fogR: Float = 0.7f,
    var fogG: Float = 0.75f,
    var fogB: Float = 0.8f,
    // DoF 参数
    var dofEnabled: Boolean = false,
    var dofFocusDist: Float = 0.5f,
    var dofNear: Float = 2.0f,
    var dofFar: Float = 20.0f,
    // Volumetric Light 参数 (god rays)
    var volumetricEnabled: Boolean = false,
    var volumetricDensity: Float = 0.03f,
    var volumetricScattering: Float = 0.75f,
    var volumetricSteps: Int = 48,
    var volumetricMaxDistance: Float = 30.0f,
    var volumetricIntensity: Float = 1.0f,
    var volumetricColorR: Float = 1.0f,
    var volumetricColorG: Float = 0.95f,
    var volumetricColorB: Float = 0.85f
) : Parcelable {
    constructor(parcel: Parcel) : this(
        aaMode = parcel.readInt(),
        bloomEnabled = parcel.readBoolean(),
        bloomIntensity = parcel.readFloat(),
        bloomThreshold = parcel.readFloat(),
        tonemapEnabled = parcel.readBoolean(),
        tonemapOp = parcel.readInt(),
        exposure = parcel.readFloat(),
        gammaCorrection = parcel.readFloat(),
        vignetteEnabled = parcel.readBoolean(),
        vignetteIntensity = parcel.readFloat(),
        fogEnabled = parcel.readBoolean(),
        fogMode = parcel.readInt(),
        fogDensity = parcel.readFloat(),
        fogStart = parcel.readFloat(),
        fogEnd = parcel.readFloat(),
        fogR = parcel.readFloat(),
        fogG = parcel.readFloat(),
        fogB = parcel.readFloat(),
        dofEnabled = parcel.readBoolean(),
        dofFocusDist = parcel.readFloat(),
        dofNear = parcel.readFloat(),
        dofFar = parcel.readFloat(),
        volumetricEnabled = parcel.readBoolean(),
        volumetricDensity = parcel.readFloat(),
        volumetricScattering = parcel.readFloat(),
        volumetricSteps = parcel.readInt(),
        volumetricMaxDistance = parcel.readFloat(),
        volumetricIntensity = parcel.readFloat(),
        volumetricColorR = parcel.readFloat(),
        volumetricColorG = parcel.readFloat(),
        volumetricColorB = parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeInt(aaMode)
        parcel.writeBoolean(bloomEnabled)
        parcel.writeFloat(bloomIntensity)
        parcel.writeFloat(bloomThreshold)
        parcel.writeBoolean(tonemapEnabled)
        parcel.writeInt(tonemapOp)
        parcel.writeFloat(exposure)
        parcel.writeFloat(gammaCorrection)
        parcel.writeBoolean(vignetteEnabled)
        parcel.writeFloat(vignetteIntensity)
        parcel.writeBoolean(fogEnabled)
        parcel.writeInt(fogMode)
        parcel.writeFloat(fogDensity)
        parcel.writeFloat(fogStart)
        parcel.writeFloat(fogEnd)
        parcel.writeFloat(fogR)
        parcel.writeFloat(fogG)
        parcel.writeFloat(fogB)
        parcel.writeBoolean(dofEnabled)
        parcel.writeFloat(dofFocusDist)
        parcel.writeFloat(dofNear)
        parcel.writeFloat(dofFar)
        parcel.writeBoolean(volumetricEnabled)
        parcel.writeFloat(volumetricDensity)
        parcel.writeFloat(volumetricScattering)
        parcel.writeInt(volumetricSteps)
        parcel.writeFloat(volumetricMaxDistance)
        parcel.writeFloat(volumetricIntensity)
        parcel.writeFloat(volumetricColorR)
        parcel.writeFloat(volumetricColorG)
        parcel.writeFloat(volumetricColorB)
    }

    override fun describeContents(): Int = 0

    companion object CREATOR : Parcelable.Creator<PostProcessConfig> {
        override fun createFromParcel(parcel: Parcel): PostProcessConfig = PostProcessConfig(parcel)
        override fun newArray(size: Int): Array<PostProcessConfig?> = arrayOfNulls(size)
    }
}

/**
 * 模型材质参数
 */
data class MaterialParams(
    var baseColor: Vec3 = Vec3(1f, 1f, 1f),
    var metallic: Float = 0f,
    var roughness: Float = 0.5f,
    var ao: Float = 1f,
    var emissive: Vec3 = Vec3(0f, 0f, 0f),
    var emissiveIntensity: Float = 0f,
    var clearcoat: Float = 0f,
    var clearcoatRoughness: Float = 0f
) : Parcelable {
    constructor(parcel: Parcel) : this(
        baseColor = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        metallic = parcel.readFloat(),
        roughness = parcel.readFloat(),
        ao = parcel.readFloat(),
        emissive = parcel.readParcelable(Vec3::class.java.classLoader) ?: Vec3(),
        emissiveIntensity = parcel.readFloat(),
        clearcoat = parcel.readFloat(),
        clearcoatRoughness = parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeParcelable(baseColor, flags)
        parcel.writeFloat(metallic)
        parcel.writeFloat(roughness)
        parcel.writeFloat(ao)
        parcel.writeParcelable(emissive, flags)
        parcel.writeFloat(emissiveIntensity)
        parcel.writeFloat(clearcoat)
        parcel.writeFloat(clearcoatRoughness)
    }

    override fun describeContents(): Int = 0

    companion object CREATOR : Parcelable.Creator<MaterialParams> {
        override fun createFromParcel(parcel: Parcel): MaterialParams = MaterialParams(parcel)
        override fun newArray(size: Int): Array<MaterialParams?> = arrayOfNulls(size)
    }
}

/**
 * 3D 向量
 */
data class Vec3(
    var x: Float = 0f,
    var y: Float = 0f,
    var z: Float = 0f
) : Parcelable {
    constructor(parcel: Parcel) : this(
        x = parcel.readFloat(),
        y = parcel.readFloat(),
        z = parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeFloat(x)
        parcel.writeFloat(y)
        parcel.writeFloat(z)
    }

    override fun describeContents(): Int = 0

    companion object CREATOR : Parcelable.Creator<Vec3> {
        override fun createFromParcel(parcel: Parcel): Vec3 = Vec3(parcel)
        override fun newArray(size: Int): Array<Vec3?> = arrayOfNulls(size)
    }
}
