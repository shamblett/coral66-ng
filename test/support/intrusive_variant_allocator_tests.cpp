/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/support/intrusive_variant_allocator.hpp"

#include <queue>

#include "lib/support/abstract_intrusive_variant.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

namespace
{
class Base : public coral66::AbstractIntrusiveVariant<Base, class Derived1, class DestrSideEffect, class Tree>
{
protected:
    template <class T>
    explicit Base(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>)
    {
    }
};

class Derived1 : public Base
{
public:
    std::size_t size;
    float thing;

    Derived1(const std::size_t size, const float thing) : Base(std::in_place_type<Derived1>), size(size), thing(thing) {}
};

class DestrSideEffect : public Base
{
public:
    inline thread_local static int i = 0;

    DestrSideEffect() : Base(std::in_place_type<DestrSideEffect>)
    {
        i = 1;
    }

    ~DestrSideEffect()
    {
        i = -1;
    }
};

class Tree : public Base
{
public:
    coral66::IVAUniquePtr<Tree> sub;

    Tree() : Base(std::in_place_type<Tree>) {}
};

} // namespace

TEST_CASE("IntrusiveVariantAllocator non trivial", "[IVA]")
{
    SECTION("Simple allocation")
    {
        coral66::IntrusiveVariantAllocator<Base> allocator;
        auto derived1 = allocator.alloc<Derived1>(5, 3.0);
        CHECK(derived1->is<Derived1>());
        CHECK(derived1->size == 5);
        CHECK(derived1->thing == 3.0);
    }
    SECTION("Does deallocate all on destruction")
    {
        DestrSideEffect::i = 0;
        {
            coral66::IntrusiveVariantAllocator<Base> allocator;
            CHECK(DestrSideEffect::i == 0);
            auto derived1 = allocator.alloc<DestrSideEffect>();
            CHECK(derived1->is<DestrSideEffect>());
            CHECK(DestrSideEffect::i == 1);
        }
        CHECK(DestrSideEffect::i == -1);
    }
    SECTION("Can manually deallocate")
    {
        DestrSideEffect::i = 0;
        coral66::IntrusiveVariantAllocator<Base> allocator;
        CHECK(DestrSideEffect::i == 0);
        auto derived1 = allocator.alloc<DestrSideEffect>();
        CHECK(derived1->is<DestrSideEffect>());
        CHECK(DestrSideEffect::i == 1);
        allocator.destroy(derived1.get());
        CHECK(DestrSideEffect::i == -1);
    }
    SECTION("Random alloc and dealloc")
    {
        std::queue<Base*> bases;
        struct Operation
        {
            bool alloc;
            std::size_t size;
            float thing;
        };
        std::vector<Operation> operations = {
            {.alloc = true, .size = 116629187, .thing = 73.9440806051959},
            {.alloc = true, .size = 277612820, .thing = 99.09547311576395},
            {.alloc = true, .size = 759545022, .thing = 3.309238435607314},
            {.alloc = true, .size = 252926135, .thing = 73.61982694682474},
            {.alloc = true, .size = 733364883, .thing = 93.6647266088162},
            {.alloc = true, .size = 760034269, .thing = 16.896271695412075},
            {.alloc = true, .size = 502155261, .thing = 83.7306164048671},
            {.alloc = true, .size = 678015504, .thing = 41.96613689884083},
            {.alloc = true, .size = 964011958, .thing = 14.894994448687825},
            {.alloc = true, .size = 229024193, .thing = 84.743393408499},
            {.alloc = true, .size = 690609869, .thing = 31.64428631906867},
            {.alloc = true, .size = 663170267, .thing = 62.80406548803369},
            {.alloc = true, .size = 809683543, .thing = 50.334987664403016},
            {.alloc = true, .size = 85605782, .thing = 69.4344477991786},
            {.alloc = true, .size = 792806898, .thing = 77.95372929739754},
            {.alloc = true, .size = 930751462, .thing = 73.00433677071945},
            {.alloc = true, .size = 160541283, .thing = 3.4544853784154297},
            {.alloc = true, .size = 20488933, .thing = 24.00270666817804},
            {.alloc = true, .size = 83645775, .thing = 63.112514531807385},
            {.alloc = true, .size = 720681039, .thing = 20.588942704253505},
            {.alloc = true, .size = 937887264, .thing = 6.370476666393468},
            {.alloc = true, .size = 283774499, .thing = 69.17255496327057},
            {.alloc = true, .size = 904687810, .thing = 91.00356977485835},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 266666161, .thing = 91.72537688339602},
            {.alloc = true, .size = 715190164, .thing = 51.191015939760874},
            {.alloc = true, .size = 701922832, .thing = 50.63408667901075},
            {.alloc = true, .size = 793357734, .thing = 87.26828544085252},
            {.alloc = true, .size = 963284151, .thing = 34.179174079132835},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 932508283, .thing = 86.58920572637314},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 73738180, .thing = 11.758922565192327},
            {.alloc = true, .size = 520193096, .thing = 99.13015702468378},
            {.alloc = true, .size = 335694452, .thing = 70.1172733959761},
            {.alloc = true, .size = 6520254, .thing = 35.16183877952068},
            {.alloc = true, .size = 311768064, .thing = 33.932052831182766},
            {.alloc = true, .size = 491211752, .thing = 60.8721763669697},
            {.alloc = true, .size = 325078653, .thing = 99.28028490687169},
            {.alloc = true, .size = 126963254, .thing = 71.19160065459147},
            {.alloc = true, .size = 298087434, .thing = 82.9604774178826},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 33475664, .thing = 22.22124535022324},
            {.alloc = true, .size = 372169970, .thing = 74.00631733651011},
            {.alloc = true, .size = 971336108, .thing = 83.14767201131882},
            {.alloc = true, .size = 571803246, .thing = 11.428300451405573},
            {.alloc = true, .size = 429952931, .thing = 77.0863175102885},
            {.alloc = true, .size = 723017655, .thing = 68.16674808497214},
            {.alloc = true, .size = 616912304, .thing = 80.52651149979516},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 880809399, .thing = 4.258850830859832},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 52400672, .thing = 43.29865002745877},
            {.alloc = true, .size = 516106192, .thing = 25.668706393029836},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 774986094, .thing = 86.1018815033307},
            {.alloc = true, .size = 360287973, .thing = 49.49569175577015},
            {.alloc = true, .size = 281939049, .thing = 39.99459246920577},
            {.alloc = true, .size = 892868611, .thing = 3.2031345299690837},
            {.alloc = true, .size = 473907923, .thing = 35.68647742645601},
            {.alloc = true, .size = 860812946, .thing = 71.72897000847412},
            {.alloc = true, .size = 552199709, .thing = 89.05936017145255},
            {.alloc = true, .size = 388388142, .thing = 7.775381070719746},
            {.alloc = true, .size = 885936333, .thing = 19.73893386933208},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 945678556, .thing = 92.1804914175329},
            {.alloc = true, .size = 938211316, .thing = 51.81772373000226},
            {.alloc = true, .size = 618728751, .thing = 12.589892778947291},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 608056883, .thing = 54.000398892571724},
            {.alloc = true, .size = 797461199, .thing = 52.29095461736943},
            {.alloc = true, .size = 917577583, .thing = 72.79425825848472},
            {.alloc = true, .size = 333101546, .thing = 9.883659339000435},
            {.alloc = true, .size = 965002092, .thing = 4.018650431478658},
            {.alloc = true, .size = 697208154, .thing = 90.64275829113174},
            {.alloc = true, .size = 585181732, .thing = 11.053402340774738},
            {.alloc = true, .size = 620345361, .thing = 99.2275038974684},
            {.alloc = true, .size = 121161793, .thing = 5.532736808901487},
            {.alloc = true, .size = 12967982, .thing = 50.05521352159485},
            {.alloc = true, .size = 669178705, .thing = 7.449067878495677},
            {.alloc = true, .size = 888993748, .thing = 30.700911291441614},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 812419646, .thing = 65.06138397286047},
            {.alloc = true, .size = 632660564, .thing = 53.402060240218695},
            {.alloc = true, .size = 657176062, .thing = 44.64627235824779},
            {.alloc = true, .size = 166542499, .thing = 72.96850750262128},
            {.alloc = true, .size = 474207958, .thing = 18.109559622205236},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 632379806, .thing = 59.867048390028636},
            {.alloc = true, .size = 893656797, .thing = 80.0382199939544},
            {.alloc = true, .size = 615634056, .thing = 50.56535231445677},
            {.alloc = true, .size = 551342756, .thing = 85.49371083096156},
            {.alloc = true, .size = 916775901, .thing = 12.488498364883696},
            {.alloc = true, .size = 807087172, .thing = 13.004933910134197},
            {.alloc = true, .size = 247875267, .thing = 94.33412960710277},
            {.alloc = true, .size = 595450484, .thing = 22.518474339334933},
            {.alloc = true, .size = 375735286, .thing = 69.14715400557094},
            {.alloc = true, .size = 147176399, .thing = 14.862978475352351},
            {.alloc = true, .size = 647235293, .thing = 21.295175348878814},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 918130805, .thing = 99.5206051732044},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 358327371, .thing = 6.19636283610596},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 484400385, .thing = 90.13967976618525},
            {.alloc = true, .size = 592041604, .thing = 31.931922984300698},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 131163117, .thing = 83.79433086682539},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 934278040, .thing = 62.39161192063027},
            {.alloc = true, .size = 298994981, .thing = 70.78561403688526},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 105368048, .thing = 99.40775626905061},
            {.alloc = true, .size = 621918159, .thing = 63.87512615564709},
            {.alloc = true, .size = 718114096, .thing = 46.97425718499749},
            {.alloc = true, .size = 199520477, .thing = 99.87551579040812},
            {.alloc = true, .size = 575745191, .thing = 54.551990390607216},
            {.alloc = true, .size = 541273913, .thing = 12.57155760395759},
            {.alloc = true, .size = 351323069, .thing = 41.30129651503819},
            {.alloc = true, .size = 218879304, .thing = 29.293405579644162},
            {.alloc = true, .size = 813470617, .thing = 77.97882158787351},
            {.alloc = true, .size = 910230605, .thing = 85.04624084604544},
            {.alloc = true, .size = 727344161, .thing = 22.802761987139217},
            {.alloc = true, .size = 155567533, .thing = 78.58022352007488},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 58288005, .thing = 35.54428757545565},
            {.alloc = true, .size = 376120208, .thing = 88.71887581633028},
            {.alloc = true, .size = 68357654, .thing = 2.3508812712750604},
            {.alloc = true, .size = 813192805, .thing = 27.982502214630777},
            {.alloc = true, .size = 607180436, .thing = 16.604026320325296},
            {.alloc = true, .size = 684023313, .thing = 62.574563960849524},
            {.alloc = true, .size = 612136167, .thing = 84.47927633093843},
            {.alloc = true, .size = 246360826, .thing = 75.3173573522723},
            {.alloc = true, .size = 391840417, .thing = 75.6411841277213},
            {.alloc = true, .size = 866640231, .thing = 59.30719311478474},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 721528691, .thing = 21.83257897155658},
            {.alloc = true, .size = 732895105, .thing = 66.42138031006863},
            {.alloc = true, .size = 48632785, .thing = 74.98457530307003},
            {.alloc = true, .size = 932788001, .thing = 84.57250869911344},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 184844282, .thing = 23.5234903730176},
            {.alloc = true, .size = 847453574, .thing = 95.96207858720149},
            {.alloc = true, .size = 621605685, .thing = 79.30698657643822},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 591846021, .thing = 67.8541830290874},
            {.alloc = true, .size = 154121936, .thing = 32.517672196522554},
            {.alloc = true, .size = 585803736, .thing = 58.22113530988516},
            {.alloc = true, .size = 336380994, .thing = 80.32309483446446},
            {.alloc = true, .size = 61788203, .thing = 58.43596840675908},
            {.alloc = true, .size = 99950288, .thing = 56.060939268453815},
            {.alloc = true, .size = 464246856, .thing = 23.86125377607018},
            {.alloc = true, .size = 62349427, .thing = 23.130488593969982},
            {.alloc = true, .size = 244304927, .thing = 65.2072363214324},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 74583073, .thing = 38.3167242044932},
            {.alloc = true, .size = 425755650, .thing = 14.120066222804123},
            {.alloc = true, .size = 895772919, .thing = 70.73947087929773},
            {.alloc = true, .size = 254317383, .thing = 82.58438480876731},
            {.alloc = true, .size = 828103278, .thing = 9.572225639387643},
            {.alloc = true, .size = 646776682, .thing = 68.18423837768886},
            {.alloc = true, .size = 428392224, .thing = 47.363134458470036},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 946493044, .thing = 56.93927884639809},
            {.alloc = true, .size = 848715073, .thing = 61.166526074781515},
            {.alloc = true, .size = 321790554, .thing = 48.41492282288697},
            {.alloc = true, .size = 131246439, .thing = 12.453650217470372},
            {.alloc = true, .size = 106820871, .thing = 59.69634962697298},
            {.alloc = true, .size = 559855813, .thing = 68.49886168111006},
            {.alloc = true, .size = 412394132, .thing = 10.156584161718932},
            {.alloc = true, .size = 77797029, .thing = 54.84962483545263},
            {.alloc = true, .size = 730561182, .thing = 17.76940249085081},
            {.alloc = true, .size = 657017712, .thing = 75.8136092298135},
            {.alloc = true, .size = 675600134, .thing = 72.31674342490885},
            {.alloc = true, .size = 722570197, .thing = 35.41388785242076},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 313319047, .thing = 68.01327788019861},
            {.alloc = true, .size = 296462873, .thing = 15.872948224213413},
            {.alloc = true, .size = 820556435, .thing = 15.019348822046947},
            {.alloc = true, .size = 305925781, .thing = 39.14654649450848},
            {.alloc = true, .size = 748951594, .thing = 19.784537848237164},
            {.alloc = true, .size = 943108333, .thing = 49.42918680801723},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 748560569, .thing = 56.674101234205914},
            {.alloc = true, .size = 712485315, .thing = 29.99890154577189},
            {.alloc = true, .size = 596708024, .thing = 76.95007715131027},
            {.alloc = true, .size = 7495410, .thing = 83.69676338698156},
            {.alloc = true, .size = 791465545, .thing = 83.78339875087184},
            {.alloc = true, .size = 347319713, .thing = 82.33423648573361},
            {.alloc = true, .size = 743820677, .thing = 63.655571529719346},
            {.alloc = true, .size = 449159274, .thing = 15.004561591098124},
            {.alloc = true, .size = 143848847, .thing = 3.76136153624694},
            {.alloc = true, .size = 442619469, .thing = 87.89209016944395},
            {.alloc = true, .size = 523578072, .thing = 32.78405342639873},
            {.alloc = true, .size = 691059371, .thing = 67.80816195830064},
            {.alloc = true, .size = 215367530, .thing = 94.12510553598761},
        };

        coral66::IntrusiveVariantAllocator<Base> allocator;
        for (std::size_t i = 0; i < 100; i++)
        {
            for (auto& [alloc, size, thing] : operations)
            {
                if (!alloc)
                {
                    if (!bases.empty())
                    {
                        allocator.destroy(bases.front());
                        bases.pop();
                    }
                    continue;
                }
                bases.push(allocator.alloc<Derived1>(size, thing).get());
                CHECK(bases.back()->is<Derived1>());
                CHECK(bases.back()->as<Derived1>().size == size);
                CHECK(bases.back()->as<Derived1>().thing == thing);
            }
        }
    }
    SECTION("Large amount")
    {
        coral66::IntrusiveVariantAllocator<Base> allocator;
        for (std::size_t i = 0; i < 1000; i++)
        {
            auto derived1 = allocator.alloc<Derived1>(5, 3.0);
            CHECK(derived1->is<Derived1>());
            CHECK(derived1->size == 5);
            CHECK(derived1->thing == 3.0);
        }
    }
    SECTION("Unique ptr")
    {
        DestrSideEffect::i = 0;
        coral66::IVAUniquePtr<DestrSideEffect> ptr;
        CHECK(DestrSideEffect::i == 0);
        coral66::IntrusiveVariantAllocator<Base> allocator;
        ptr = allocator.allocUnique<DestrSideEffect>();
        CHECK(ptr->is<DestrSideEffect>());
        CHECK(DestrSideEffect::i == 1);
        ptr.reset();
        CHECK(DestrSideEffect::i == -1);
    }
    SECTION("No double destruction")
    {
        coral66::IntrusiveVariantAllocator<Base> allocator;
        auto firstTree = allocator.allocUnique<Tree>();
        auto secondTree = allocator.allocUnique<Tree>();
        secondTree->sub = std::move(firstTree);
        secondTree.release(); // LEAK so to say, but allocator will clean it up
    }
}

namespace
{
class Base2 : public coral66::AbstractIntrusiveVariant<Base2, class Derived2>
{
protected:
    template <class T>
    explicit Base2(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>)
    {
    }
};

class Derived2 : public Base2
{
public:
    std::size_t size;
    float thing;

    Derived2(const std::size_t size, const float thing) : Base2(std::in_place_type<Derived2>), size(size), thing(thing) {}
};

} // namespace

TEST_CASE("IntrusiveVariantAllocator trivial", "[IVA]")
{
    SECTION("Simple allocation")
    {
        coral66::IntrusiveVariantAllocator<Base2> allocator;
        auto derived1 = allocator.alloc<Derived2>(5, 3.0);
        CHECK(derived1->is<Derived2>());
        CHECK(derived1->size == 5);
        CHECK(derived1->thing == 3.0);
    }
    SECTION("Random alloc and dealloc")
    {
        std::queue<Base2*> bases;
        struct Operation
        {
            bool alloc;
            std::size_t size;
            float thing;
        };
        std::vector<Operation> operations = {
            {.alloc = true, .size = 116629187, .thing = 73.9440806051959},
            {.alloc = true, .size = 277612820, .thing = 99.09547311576395},
            {.alloc = true, .size = 759545022, .thing = 3.309238435607314},
            {.alloc = true, .size = 252926135, .thing = 73.61982694682474},
            {.alloc = true, .size = 733364883, .thing = 93.6647266088162},
            {.alloc = true, .size = 760034269, .thing = 16.896271695412075},
            {.alloc = true, .size = 502155261, .thing = 83.7306164048671},
            {.alloc = true, .size = 678015504, .thing = 41.96613689884083},
            {.alloc = true, .size = 964011958, .thing = 14.894994448687825},
            {.alloc = true, .size = 229024193, .thing = 84.743393408499},
            {.alloc = true, .size = 690609869, .thing = 31.64428631906867},
            {.alloc = true, .size = 663170267, .thing = 62.80406548803369},
            {.alloc = true, .size = 809683543, .thing = 50.334987664403016},
            {.alloc = true, .size = 85605782, .thing = 69.4344477991786},
            {.alloc = true, .size = 792806898, .thing = 77.95372929739754},
            {.alloc = true, .size = 930751462, .thing = 73.00433677071945},
            {.alloc = true, .size = 160541283, .thing = 3.4544853784154297},
            {.alloc = true, .size = 20488933, .thing = 24.00270666817804},
            {.alloc = true, .size = 83645775, .thing = 63.112514531807385},
            {.alloc = true, .size = 720681039, .thing = 20.588942704253505},
            {.alloc = true, .size = 937887264, .thing = 6.370476666393468},
            {.alloc = true, .size = 283774499, .thing = 69.17255496327057},
            {.alloc = true, .size = 904687810, .thing = 91.00356977485835},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 266666161, .thing = 91.72537688339602},
            {.alloc = true, .size = 715190164, .thing = 51.191015939760874},
            {.alloc = true, .size = 701922832, .thing = 50.63408667901075},
            {.alloc = true, .size = 793357734, .thing = 87.26828544085252},
            {.alloc = true, .size = 963284151, .thing = 34.179174079132835},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 932508283, .thing = 86.58920572637314},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 73738180, .thing = 11.758922565192327},
            {.alloc = true, .size = 520193096, .thing = 99.13015702468378},
            {.alloc = true, .size = 335694452, .thing = 70.1172733959761},
            {.alloc = true, .size = 6520254, .thing = 35.16183877952068},
            {.alloc = true, .size = 311768064, .thing = 33.932052831182766},
            {.alloc = true, .size = 491211752, .thing = 60.8721763669697},
            {.alloc = true, .size = 325078653, .thing = 99.28028490687169},
            {.alloc = true, .size = 126963254, .thing = 71.19160065459147},
            {.alloc = true, .size = 298087434, .thing = 82.9604774178826},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 33475664, .thing = 22.22124535022324},
            {.alloc = true, .size = 372169970, .thing = 74.00631733651011},
            {.alloc = true, .size = 971336108, .thing = 83.14767201131882},
            {.alloc = true, .size = 571803246, .thing = 11.428300451405573},
            {.alloc = true, .size = 429952931, .thing = 77.0863175102885},
            {.alloc = true, .size = 723017655, .thing = 68.16674808497214},
            {.alloc = true, .size = 616912304, .thing = 80.52651149979516},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 880809399, .thing = 4.258850830859832},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 52400672, .thing = 43.29865002745877},
            {.alloc = true, .size = 516106192, .thing = 25.668706393029836},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 774986094, .thing = 86.1018815033307},
            {.alloc = true, .size = 360287973, .thing = 49.49569175577015},
            {.alloc = true, .size = 281939049, .thing = 39.99459246920577},
            {.alloc = true, .size = 892868611, .thing = 3.2031345299690837},
            {.alloc = true, .size = 473907923, .thing = 35.68647742645601},
            {.alloc = true, .size = 860812946, .thing = 71.72897000847412},
            {.alloc = true, .size = 552199709, .thing = 89.05936017145255},
            {.alloc = true, .size = 388388142, .thing = 7.775381070719746},
            {.alloc = true, .size = 885936333, .thing = 19.73893386933208},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 945678556, .thing = 92.1804914175329},
            {.alloc = true, .size = 938211316, .thing = 51.81772373000226},
            {.alloc = true, .size = 618728751, .thing = 12.589892778947291},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 608056883, .thing = 54.000398892571724},
            {.alloc = true, .size = 797461199, .thing = 52.29095461736943},
            {.alloc = true, .size = 917577583, .thing = 72.79425825848472},
            {.alloc = true, .size = 333101546, .thing = 9.883659339000435},
            {.alloc = true, .size = 965002092, .thing = 4.018650431478658},
            {.alloc = true, .size = 697208154, .thing = 90.64275829113174},
            {.alloc = true, .size = 585181732, .thing = 11.053402340774738},
            {.alloc = true, .size = 620345361, .thing = 99.2275038974684},
            {.alloc = true, .size = 121161793, .thing = 5.532736808901487},
            {.alloc = true, .size = 12967982, .thing = 50.05521352159485},
            {.alloc = true, .size = 669178705, .thing = 7.449067878495677},
            {.alloc = true, .size = 888993748, .thing = 30.700911291441614},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 812419646, .thing = 65.06138397286047},
            {.alloc = true, .size = 632660564, .thing = 53.402060240218695},
            {.alloc = true, .size = 657176062, .thing = 44.64627235824779},
            {.alloc = true, .size = 166542499, .thing = 72.96850750262128},
            {.alloc = true, .size = 474207958, .thing = 18.109559622205236},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 632379806, .thing = 59.867048390028636},
            {.alloc = true, .size = 893656797, .thing = 80.0382199939544},
            {.alloc = true, .size = 615634056, .thing = 50.56535231445677},
            {.alloc = true, .size = 551342756, .thing = 85.49371083096156},
            {.alloc = true, .size = 916775901, .thing = 12.488498364883696},
            {.alloc = true, .size = 807087172, .thing = 13.004933910134197},
            {.alloc = true, .size = 247875267, .thing = 94.33412960710277},
            {.alloc = true, .size = 595450484, .thing = 22.518474339334933},
            {.alloc = true, .size = 375735286, .thing = 69.14715400557094},
            {.alloc = true, .size = 147176399, .thing = 14.862978475352351},
            {.alloc = true, .size = 647235293, .thing = 21.295175348878814},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 918130805, .thing = 99.5206051732044},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 358327371, .thing = 6.19636283610596},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 484400385, .thing = 90.13967976618525},
            {.alloc = true, .size = 592041604, .thing = 31.931922984300698},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 131163117, .thing = 83.79433086682539},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 934278040, .thing = 62.39161192063027},
            {.alloc = true, .size = 298994981, .thing = 70.78561403688526},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 105368048, .thing = 99.40775626905061},
            {.alloc = true, .size = 621918159, .thing = 63.87512615564709},
            {.alloc = true, .size = 718114096, .thing = 46.97425718499749},
            {.alloc = true, .size = 199520477, .thing = 99.87551579040812},
            {.alloc = true, .size = 575745191, .thing = 54.551990390607216},
            {.alloc = true, .size = 541273913, .thing = 12.57155760395759},
            {.alloc = true, .size = 351323069, .thing = 41.30129651503819},
            {.alloc = true, .size = 218879304, .thing = 29.293405579644162},
            {.alloc = true, .size = 813470617, .thing = 77.97882158787351},
            {.alloc = true, .size = 910230605, .thing = 85.04624084604544},
            {.alloc = true, .size = 727344161, .thing = 22.802761987139217},
            {.alloc = true, .size = 155567533, .thing = 78.58022352007488},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 58288005, .thing = 35.54428757545565},
            {.alloc = true, .size = 376120208, .thing = 88.71887581633028},
            {.alloc = true, .size = 68357654, .thing = 2.3508812712750604},
            {.alloc = true, .size = 813192805, .thing = 27.982502214630777},
            {.alloc = true, .size = 607180436, .thing = 16.604026320325296},
            {.alloc = true, .size = 684023313, .thing = 62.574563960849524},
            {.alloc = true, .size = 612136167, .thing = 84.47927633093843},
            {.alloc = true, .size = 246360826, .thing = 75.3173573522723},
            {.alloc = true, .size = 391840417, .thing = 75.6411841277213},
            {.alloc = true, .size = 866640231, .thing = 59.30719311478474},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 721528691, .thing = 21.83257897155658},
            {.alloc = true, .size = 732895105, .thing = 66.42138031006863},
            {.alloc = true, .size = 48632785, .thing = 74.98457530307003},
            {.alloc = true, .size = 932788001, .thing = 84.57250869911344},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 184844282, .thing = 23.5234903730176},
            {.alloc = true, .size = 847453574, .thing = 95.96207858720149},
            {.alloc = true, .size = 621605685, .thing = 79.30698657643822},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 591846021, .thing = 67.8541830290874},
            {.alloc = true, .size = 154121936, .thing = 32.517672196522554},
            {.alloc = true, .size = 585803736, .thing = 58.22113530988516},
            {.alloc = true, .size = 336380994, .thing = 80.32309483446446},
            {.alloc = true, .size = 61788203, .thing = 58.43596840675908},
            {.alloc = true, .size = 99950288, .thing = 56.060939268453815},
            {.alloc = true, .size = 464246856, .thing = 23.86125377607018},
            {.alloc = true, .size = 62349427, .thing = 23.130488593969982},
            {.alloc = true, .size = 244304927, .thing = 65.2072363214324},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 74583073, .thing = 38.3167242044932},
            {.alloc = true, .size = 425755650, .thing = 14.120066222804123},
            {.alloc = true, .size = 895772919, .thing = 70.73947087929773},
            {.alloc = true, .size = 254317383, .thing = 82.58438480876731},
            {.alloc = true, .size = 828103278, .thing = 9.572225639387643},
            {.alloc = true, .size = 646776682, .thing = 68.18423837768886},
            {.alloc = true, .size = 428392224, .thing = 47.363134458470036},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 946493044, .thing = 56.93927884639809},
            {.alloc = true, .size = 848715073, .thing = 61.166526074781515},
            {.alloc = true, .size = 321790554, .thing = 48.41492282288697},
            {.alloc = true, .size = 131246439, .thing = 12.453650217470372},
            {.alloc = true, .size = 106820871, .thing = 59.69634962697298},
            {.alloc = true, .size = 559855813, .thing = 68.49886168111006},
            {.alloc = true, .size = 412394132, .thing = 10.156584161718932},
            {.alloc = true, .size = 77797029, .thing = 54.84962483545263},
            {.alloc = true, .size = 730561182, .thing = 17.76940249085081},
            {.alloc = true, .size = 657017712, .thing = 75.8136092298135},
            {.alloc = true, .size = 675600134, .thing = 72.31674342490885},
            {.alloc = true, .size = 722570197, .thing = 35.41388785242076},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 313319047, .thing = 68.01327788019861},
            {.alloc = true, .size = 296462873, .thing = 15.872948224213413},
            {.alloc = true, .size = 820556435, .thing = 15.019348822046947},
            {.alloc = true, .size = 305925781, .thing = 39.14654649450848},
            {.alloc = true, .size = 748951594, .thing = 19.784537848237164},
            {.alloc = true, .size = 943108333, .thing = 49.42918680801723},
            {.alloc = false, .size = 0, .thing = 0.0f},
            {.alloc = true, .size = 748560569, .thing = 56.674101234205914},
            {.alloc = true, .size = 712485315, .thing = 29.99890154577189},
            {.alloc = true, .size = 596708024, .thing = 76.95007715131027},
            {.alloc = true, .size = 7495410, .thing = 83.69676338698156},
            {.alloc = true, .size = 791465545, .thing = 83.78339875087184},
            {.alloc = true, .size = 347319713, .thing = 82.33423648573361},
            {.alloc = true, .size = 743820677, .thing = 63.655571529719346},
            {.alloc = true, .size = 449159274, .thing = 15.004561591098124},
            {.alloc = true, .size = 143848847, .thing = 3.76136153624694},
            {.alloc = true, .size = 442619469, .thing = 87.89209016944395},
            {.alloc = true, .size = 523578072, .thing = 32.78405342639873},
            {.alloc = true, .size = 691059371, .thing = 67.80816195830064},
            {.alloc = true, .size = 215367530, .thing = 94.12510553598761},
        };

        coral66::IntrusiveVariantAllocator<Base2> allocator;
        for (std::size_t i = 0; i < 100; i++)
        {
            for (auto& [alloc, size, thing] : operations)
            {
                if (!alloc)
                {
                    if (!bases.empty())
                    {
                        allocator.destroy(bases.front());
                        bases.pop();
                    }
                    continue;
                }
                bases.push(allocator.alloc<Derived2>(size, thing).get());
                CHECK(bases.back()->is<Derived2>());
                CHECK(bases.back()->as<Derived2>().size == size);
                CHECK(bases.back()->as<Derived2>().thing == thing);
            }
        }
    }
    SECTION("Large amount")
    {
        coral66::IntrusiveVariantAllocator<Base2> allocator;
        for (std::size_t i = 0; i < 1000; i++)
        {
            auto derived1 = allocator.alloc<Derived2>(5, 3.0);
            CHECK(derived1->is<Derived2>());
            CHECK(derived1->size == 5);
            CHECK(derived1->thing == 3.0);
        }
    }
}
