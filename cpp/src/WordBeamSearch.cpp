#include "WordBeamSearch.hpp"
#include "Beam.hpp"
#include <vector>
#include <memory>

#include <iostream>


std::vector<uint32_t> wordBeamSearch(const IMatrix& mat, size_t beamWidth, const std::shared_ptr<LanguageModel>& lm, LanguageModelType lmType)
{
	// dim0: T, dim1: C
	const size_t maxT = mat.rows();
	const size_t maxC = mat.cols(); //REMOVE
	const size_t blank = 0;//maxC - 1;


	// initialise with genesis beam
	BeamList curr;
	BeamList last;
	const bool useNGrams = lmType == LanguageModelType::NGrams || lmType == LanguageModelType::NGramsForecast || lmType==LanguageModelType::NGramsForecastAndSample;
	const bool forcastNGrams = lmType == LanguageModelType::NGramsForecast || lmType == LanguageModelType::NGramsForecastAndSample;
	const bool sampleNGrams = lmType == LanguageModelType::NGramsForecastAndSample;
	last.addBeam(std::make_shared<Beam>(lm, useNGrams, forcastNGrams, sampleNGrams));


	// go over all time steps
	for (size_t t = 0; t < maxT; ++t)
	{
		//std::cout << "POINT B: " << t << std::endl;
		// get k best beams and iterate 
		const std::vector<std::shared_ptr<Beam>> bestBeams = last.getBestBeams(beamWidth);
		for (const auto& beam : bestBeams)
		{
			double prBlank=0.0, prNonBlank=0.0;

			// calc prob that path ends with a non-blank
			prNonBlank = beam->isEmpty() ? 0.0 : beam->getNonBlankProb() * mat.getAt(t, beam->getLastChar());

			// calc prob that path ends with a blank
			prBlank = beam->getTotalProb() * mat.getAt(t, blank);
			

			auto extender = (beam->isEmpty() || (mat.getAt(t, beam->getLastChar()) < mat.getAt(t, blank))) ? blank : beam->getLastChar();

			// add copy of original beam to current time step
			curr.addBeam(beam->createChildBeam(prBlank, prNonBlank, extender));

			//std::cout << "POINT C" << std::endl;

			// extend current beam
			const std::vector<uint32_t> nextChars = beam->getNextChars();
			for (const auto c : nextChars)
			{
				//std::cout << "POINT D: " << t << ' ' << c << std::endl;
				prBlank = 0.0;
				prNonBlank = 0.0;
				// last char in beam equals new char: path must end with blank
				if (!beam->isEmpty() && beam->getLastChar() == c)
				{
					prNonBlank = mat.getAt(t, c) * beam->getBlankProb();
				}
				// last char in beam and new char different
				else
				{
					prNonBlank = mat.getAt(t, c) * beam->getTotalProb();
				}

				curr.addBeam(beam->createChildBeam(prBlank, prNonBlank, c, c));
			}
		}

		last = std::move(curr);
	}

	// return best entry
	const auto bestBeam = last.getBestBeams(1)[0];
//	bestBeam->completeText();
	return bestBeam->getText();
}


