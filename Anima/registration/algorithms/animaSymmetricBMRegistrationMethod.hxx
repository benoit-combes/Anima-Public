#pragma once
#include "animaSymmetricBMRegistrationMethod.h"

#include <animaBalooSVFTransformAgregator.h>
#include <animaDenseSVFTransformAgregator.h>

namespace anima
{

template <typename TInputImageType>
void
SymmetricBMRegistrationMethod <TInputImageType>
::PerformOneIteration(InputImageType *refImage, InputImageType *movingImage, TransformPointer &addOn)
{
    itk::TimeProbe tmpTime;
    tmpTime.Start();

    this->GetBlockMatcher()->SetForceComputeBlocks(false);
    this->GetBlockMatcher()->SetReferenceImage(this->GetFixedImage());
    this->GetBlockMatcher()->SetMovingImage(movingImage);
    this->GetBlockMatcher()->SetNumberOfWorkUnits(this->GetNumberOfWorkUnits());
    this->GetBlockMatcher()->Update();

    tmpTime.Stop();

    if (this->GetVerboseProgression())
        std::cout << "Matching performed in " << tmpTime.GetTotal() << std::endl;

    this->GetAgregator()->SetInputRegions(this->GetBlockMatcher()->GetBlockRegions());
    this->GetAgregator()->SetInputOrigins(this->GetBlockMatcher()->GetBlockPositions());
    this->GetAgregator()->SetInputWeights(this->GetBlockMatcher()->GetBlockWeights());
    this->GetAgregator()->SetInputTransforms(this->GetBlockMatcher()->GetBlockTransformPointers());

    TransformPointer usualAddOn = this->GetAgregator()->GetOutput();

    itk::TimeProbe tmpTimeReverse;
    tmpTimeReverse.Start();

    m_ReverseBlockMatcher->SetForceComputeBlocks(false);
    m_ReverseBlockMatcher->SetReferenceImage(this->GetMovingImage());
    m_ReverseBlockMatcher->SetMovingImage(refImage);
    m_ReverseBlockMatcher->SetNumberOfWorkUnits(this->GetNumberOfWorkUnits());
    m_ReverseBlockMatcher->Update();

    tmpTimeReverse.Stop();

    if (this->GetVerboseProgression())
        std::cout << "Matching performed in " << tmpTimeReverse.GetTotal() << std::endl;

    this->GetAgregator()->SetInputRegions(m_ReverseBlockMatcher->GetBlockRegions());
    this->GetAgregator()->SetInputOrigins(m_ReverseBlockMatcher->GetBlockPositions());
    this->GetAgregator()->SetInputWeights(m_ReverseBlockMatcher->GetBlockWeights());
    this->GetAgregator()->SetInputTransforms(m_ReverseBlockMatcher->GetBlockTransformPointers());

    TransformPointer reverseAddOn = this->GetAgregator()->GetOutput();

    if (this->GetAgregator()->GetOutputTransformType() == AgregatorType::SVF)
    {
        // Add update to current velocity field (cf. Vercauteren et al, 2008)
        // First compute the SVF from two asymmetric ones: S = 0.5 * (S_0 - S_1)
        // It's 0.5 since we are computing the full transform between the two images
        typedef typename SVFTransformType::VectorFieldType VelocityFieldType;

        typedef typename itk::ImageRegionConstIterator <VelocityFieldType> VelocityFieldConstIterator;
        typedef typename itk::ImageRegionIterator <VelocityFieldType> VelocityFieldIterator;

        SVFTransformType *usualAddOnCast = dynamic_cast <SVFTransformType *> (usualAddOn.GetPointer());
        SVFTransformType *reverseAddOnCast = dynamic_cast <SVFTransformType *> (reverseAddOn.GetPointer());

        typename VelocityFieldType::Pointer usualAddOnSVF = const_cast <VelocityFieldType *> (usualAddOnCast->GetParametersAsVectorField());

        VelocityFieldIterator usualAddOnItr(usualAddOnSVF,usualAddOnSVF->GetLargestPossibleRegion());

        VelocityFieldConstIterator reverseAddOnItr(reverseAddOnCast->GetParametersAsVectorField(),
                                                   reverseAddOnCast->GetParametersAsVectorField()->GetLargestPossibleRegion());

        typedef typename VelocityFieldType::PixelType VectorType;
        VectorType tmpVec;
        while (!usualAddOnItr.IsAtEnd())
        {
            tmpVec = 0.5 * (usualAddOnItr.Get() - reverseAddOnItr.Get());
            usualAddOnItr.Set(tmpVec);

            ++usualAddOnItr;
            ++reverseAddOnItr;
        }
    }
    else
    {
        AffineTransformType *usualAddOnCast = dynamic_cast <AffineTransformType *> (usualAddOn.GetPointer());
        AffineTransformType *reverseAddOnCast = dynamic_cast <AffineTransformType *> (reverseAddOn.GetPointer());

        unsigned int NDimensions = InputImageType::ImageDimension;
        vnl_matrix <double> usualAddOnMatrix(NDimensions+1,NDimensions+1,0);
        vnl_matrix <double> reverseAddOnMatrix(NDimensions+1,NDimensions+1,0);
        usualAddOnMatrix.set_identity();
        reverseAddOnMatrix.set_identity();

        for (unsigned int i = 0;i < NDimensions;++i)
        {
            for (unsigned int j = 0;j < NDimensions;++j)
            {
                usualAddOnMatrix(i,j) = usualAddOnCast->GetMatrix()(i,j);
                reverseAddOnMatrix(i,j) = reverseAddOnCast->GetMatrix()(i,j);
            }

            usualAddOnMatrix(i,NDimensions) = usualAddOnCast->GetOffset()[i];
            reverseAddOnMatrix(i,NDimensions) = reverseAddOnCast->GetOffset()[i];
        }

        usualAddOnMatrix = anima::GetLogarithm(usualAddOnMatrix);
        reverseAddOnMatrix = anima::GetLogarithm(reverseAddOnMatrix);

        usualAddOnMatrix -= reverseAddOnMatrix;
        usualAddOnMatrix /= 2.0;

        usualAddOnMatrix = anima::GetExponential(usualAddOnMatrix);

        typename AffineTransformType::MatrixType trsfMatrix;

        for (unsigned int i = 0;i < NDimensions;++i)
            for (unsigned int j = 0;j < NDimensions;++j)
                trsfMatrix(i,j) = usualAddOnMatrix(i,j);

        usualAddOnCast->SetMatrix(trsfMatrix);

        typename AffineTransformType::OffsetType trsfOffset;
        for (unsigned int i = 0;i < NDimensions;++i)
            trsfOffset[i] = usualAddOnMatrix(i,NDimensions);

        usualAddOnCast->SetOffset(trsfOffset);
    }

    addOn = usualAddOn;
}

}
