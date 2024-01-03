#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "../lve_Ipre_processing.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_texture.hpp"
#include "waveGenerationSystems/wave_InverseHFFT.hpp"
#include "waveGenerationSystems/wave_InverseVFFT.hpp"
#include "waveGenerationSystems/wave_Permute.hpp"
#include "waveGenerationSystems/wave_TimeUpdate.hpp"
#include "waveGenerationSystems/wave_conjugate.hpp"
#include "waveGenerationSystems/wave_merge.hpp"
#include "waveGenerationSystems/wave_spectrum.hpp"

namespace lve {
/**
* Cette classe et toute les classes qui lui sont associées sont une réimplémentation de l'algorithme de génération de vagues de Jump Trajectory (https://www.youtube.com/watch?v=kGEqaX4Y4bQ)
* Je me suis aidé de son code source pour comprendre les document de recherche sur JONSWAP ainsi que de la transformation inverse de fourier affin de le réimplémenter en C++ et Vulkan
*/
class WaveGen : public LveIPreProcessing {
   public:
    WaveGen(LveDevice &device, float LengthScale, float CutoffLow, float CutoffHigh);
    ~WaveGen();

    void executePreCpS(FrameInfo FrameInfo) override;

    std::shared_ptr<LveTexture> getDisplacement() { return displacement[0]; }

    std::shared_ptr<LveTexture> getDerivatives() { return derivatives[0]; }

    std::shared_ptr<LveTexture> getTurbulence() { return turbulence[0]; }

    std::vector<std::shared_ptr<LveTexture>> getAllDisplacement() { return displacement; }

    std::vector<std::shared_ptr<LveTexture>> getAllDerivatives() { return derivatives; }

    std::vector<std::shared_ptr<LveTexture>> getAllTurbulence() { return turbulence; }

   private:
    void CalculateInitial(FrameInfo FrameInfo);
    void createTextures();
    void createdescriptorSet();

    void copySpectrumTexture();

    bool DataIsUpdate = true;

    std::vector<float> loadPrecomputeData();

    std::shared_ptr<LveTexture> spectrumTexture;
    std::vector<std::shared_ptr<LveTexture>> spectrumTextureCopy1;
    std::shared_ptr<LveTexture> waveDataTexture;
    std::shared_ptr<LveTexture> spectrumConjugateTexture;
    std::vector<std::shared_ptr<LveTexture>> DxDz;
    std::vector<std::shared_ptr<LveTexture>> DyDxz;
    std::vector<std::shared_ptr<LveTexture>> DyxDyz;
    std::vector<std::shared_ptr<LveTexture>> DxxDzz;
    std::vector<std::shared_ptr<LveTexture>> displacement;
    std::vector<std::shared_ptr<LveTexture>> derivatives;
    std::vector<std::shared_ptr<LveTexture>> turbulence;

    std::shared_ptr<LveTexture> preComputeData;

    std::unique_ptr<WaveConjugate> waveConjugate;

    std::unique_ptr<WaveVertIFFT> waveVertIFFTDxDz;
    std::unique_ptr<WaveHorIFFT> waveHorIFFTDxDz;
    std::unique_ptr<WaveVertIFFT> waveVertIFFTDyDxz;
    std::unique_ptr<WaveHorIFFT> waveHorIFFTDyDxz;
    std::unique_ptr<WaveVertIFFT> waveVertIFFTDyxDyz;
    std::unique_ptr<WaveHorIFFT> waveHorIFFTDyxDyz;
    std::unique_ptr<WaveVertIFFT> waveVertIFFTDxxDzz;
    std::unique_ptr<WaveHorIFFT> waveHorIFFTDxxDzz;

    std::unique_ptr<WaveMerge> waveMerge;
    std::unique_ptr<WavePermute> wavePermuteDxDz;
    std::unique_ptr<WavePermute> wavePermuteDyDxz;
    std::unique_ptr<WavePermute> wavePermuteDyxDyz;
    std::unique_ptr<WavePermute> wavePermuteDxxDzz;
    std::unique_ptr<WaveSpectrum> waveTextureGenerator;
    std::unique_ptr<WaveTimeUpdate> waveTimeUpdate;

    LveDevice &lveDevice;
};
}  // namespace lve