#pragma once

#include <iostream>
#include <animaMaskedImageToImageFilter.h>
#include <itkVectorImage.h>
#include <itkImage.h>

#include <vector>

namespace anima
{

template <class PixelScalarType>
class LocalPatchMeanDistanceImageFilter :
public anima::MaskedImageToImageFilter< itk::VectorImage <PixelScalarType, 3> , itk::Image <PixelScalarType, 3> >
{
public:
    /** Standard class typedefs. */
    typedef LocalPatchMeanDistanceImageFilter<PixelScalarType> Self;
    typedef itk::SmartPointer<Self> Pointer;
    typedef itk::SmartPointer<const Self>  ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods) */
    itkTypeMacro(LocalPatchMeanDistanceImageFilter, MaskedImageToImageFilter)

    /** Image typedef support */
    typedef itk::VectorImage <PixelScalarType, 3> InputImageType;
    typedef itk::Image <PixelScalarType, 3> OutputImageType;

    typedef typename InputImageType::PixelType VectorType;

    typedef vnl_matrix <double> CovarianceType;

    typedef typename InputImageType::Pointer InputImagePointer;
    typedef typename InputImageType::IndexType InputImageIndexType;
    typedef typename OutputImageType::Pointer OutputImagePointer;

    /** Superclass typedefs. */
    typedef anima::MaskedImageToImageFilter< InputImageType, OutputImageType > Superclass;
    typedef typename Superclass::MaskImageType MaskImageType;
    typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

    itkSetMacro(PatchHalfSize, unsigned int)

protected:
    LocalPatchMeanDistanceImageFilter()
    : Superclass()
    {
        this->SetNumberOfRequiredOutputs(2);
        this->SetNthOutput(0,this->MakeOutput(0));
        this->SetNthOutput(1,this->MakeOutput(1));

        m_PatchHalfSize = 1;
    }

    virtual ~LocalPatchMeanDistanceImageFilter() {}

    void BeforeThreadedGenerateData() ITK_OVERRIDE;
    void DynamicThreadedGenerateData(const OutputImageRegionType &outputRegionForThread) ITK_OVERRIDE;

private:
    ITK_DISALLOW_COPY_AND_ASSIGN(LocalPatchMeanDistanceImageFilter);

    unsigned int m_PatchHalfSize;
};

} // end namespace anima

#include "animaLocalPatchMeanDistanceImageFilter.hxx"
