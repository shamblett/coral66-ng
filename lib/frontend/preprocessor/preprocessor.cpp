/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "preprocessor.hpp"

#include <algorithm>
#include <vector>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/support/scope_exit.hpp"

namespace
{

} // namespace

coral66::PPSourceObject coral66::pp::preprocess(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter,
                                                bool* errorsOccurred) noexcept
{
    Preprocessor preprocessor(reporter);
    preprocessor.process(std::move(sourceObject));
    if (errorsOccurred)
    {
        *errorsOccurred = preprocessor.errorsOccurred();
    }
    return {std::move(preprocessor.getResult()),
            std::move(preprocessor.getFiles()),
            {preprocessor.getSubstitutionsVector().begin(), preprocessor.getSubstitutionsVector().end()},
            {std::move(preprocessor.getIntervalMaps())}};
}
