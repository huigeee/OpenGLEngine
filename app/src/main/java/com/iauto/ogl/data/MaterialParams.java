package com.iauto.ogl.data;

import android.os.Parcel;
import android.os.Parcelable;

public class MaterialParams implements Parcelable {
    private Vec3 baseColor;
    private float metallic, roughness, ao;
    private Vec3 emissive;
    private float emissiveIntensity;
    private float clearcoat, clearcoatRoughness;

    public MaterialParams() {
        this(new Vec3(1f, 1f, 1f), 0f, 0.5f, 1f, new Vec3(0f, 0f, 0f), 0f, 0f, 0f);
    }
    public MaterialParams(Vec3 baseColor, float metallic, float roughness, float ao,
                          Vec3 emissive, float emissiveIntensity, float clearcoat, float clearcoatRoughness) {
        this.baseColor = baseColor; this.metallic = metallic; this.roughness = roughness; this.ao = ao;
        this.emissive = emissive; this.emissiveIntensity = emissiveIntensity;
        this.clearcoat = clearcoat; this.clearcoatRoughness = clearcoatRoughness;
    }
    protected MaterialParams(Parcel parcel) {
        ClassLoader cl = Vec3.class.getClassLoader();
        baseColor = parcel.readParcelable(cl); if (baseColor == null) baseColor = new Vec3();
        metallic = parcel.readFloat(); roughness = parcel.readFloat(); ao = parcel.readFloat();
        emissive = parcel.readParcelable(cl); if (emissive == null) emissive = new Vec3();
        emissiveIntensity = parcel.readFloat(); clearcoat = parcel.readFloat(); clearcoatRoughness = parcel.readFloat();
    }

    public Vec3 getBaseColor() { return baseColor; }
    public void setBaseColor(Vec3 v) { this.baseColor = v; }
    public float getMetallic() { return metallic; }
    public void setMetallic(float v) { this.metallic = v; }
    public float getRoughness() { return roughness; }
    public void setRoughness(float v) { this.roughness = v; }
    public float getAo() { return ao; }
    public void setAo(float v) { this.ao = v; }
    public Vec3 getEmissive() { return emissive; }
    public void setEmissive(Vec3 v) { this.emissive = v; }
    public float getEmissiveIntensity() { return emissiveIntensity; }
    public void setEmissiveIntensity(float v) { this.emissiveIntensity = v; }
    public float getClearcoat() { return clearcoat; }
    public void setClearcoat(float v) { this.clearcoat = v; }
    public float getClearcoatRoughness() { return clearcoatRoughness; }
    public void setClearcoatRoughness(float v) { this.clearcoatRoughness = v; }

    @Override public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(baseColor, flags); dest.writeFloat(metallic); dest.writeFloat(roughness); dest.writeFloat(ao);
        dest.writeParcelable(emissive, flags); dest.writeFloat(emissiveIntensity);
        dest.writeFloat(clearcoat); dest.writeFloat(clearcoatRoughness);
    }
    @Override public int describeContents() { return 0; }

    public static final Parcelable.Creator<MaterialParams> CREATOR = new Parcelable.Creator<MaterialParams>() {
        @Override public MaterialParams createFromParcel(Parcel source) { return new MaterialParams(source); }
        @Override public MaterialParams[] newArray(int size) { return new MaterialParams[size]; }
    };
}
