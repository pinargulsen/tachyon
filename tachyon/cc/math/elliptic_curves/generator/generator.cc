#include "absl/strings/str_replace.h"

#include "tachyon/base/console/iostream.h"
#include "tachyon/base/files/file_path_flag.h"
#include "tachyon/base/flag/flag_parser.h"
#include "tachyon/build/generator_util.h"
#include "tachyon/base/strings/string_util.h"
#include "tachyon/build/cc_writer.h"

namespace tachyon {

std::string_view kPrimeFieldSuffices[] = {"fq", "fr"};

std::string_view kPointSuffices[] = {
    "g1_affine", "g1_projective", "g1_jacobian",
    "g1_xyzz",   "g1_point2",     "g1_point3",
};

std::string_view kInternalTypes[] = {
    "AffinePoint", "ProjectivePoint", "JacobianPoint",
    "PointXYZZ",   "Point2",          "Point3",
};

int kPointDimensions[] = {2, 3, 3, 4, 2, 3};

std::string GetLocation(std::string_view type) {
  if (type == "bn254") {
    return "bn/bn254";
  } else if (type == "bls12_381") {
    return "bls/bls12_381";
  }
  NOTREACHED() << "Unsupported type: " << type;
  return "";
}

struct GenerationConfig : public build::CcWriter {
  std::string type;
  int fq_limb_nums;
  int fr_limb_nums;

  int GeneratePointTraitsHdr() const;
  int GenerateUtilHdr() const;
  int GenerateUtilSrc() const;
};

int GenerationConfig::GeneratePointTraitsHdr() const {
  std::vector<std::string_view> tpl = {
      "#include \"tachyon/c/math/elliptic_curves/%{header_dir_name}/fr.h\"",
      "#include \"tachyon/c/math/elliptic_curves/%{header_dir_name}/g1.h\"",
      "#include \"tachyon/cc/math/elliptic_curves/point_traits.h\"",
      "#include \"tachyon/cc/math/finite_fields/prime_field_traits.h\"",
      "#include \"tachyon/math/elliptic_curves/%{header_dir_name}/g1.h\"",
      "",
      "namespace tachyon::cc::math {",
      "",
      "template <>",
      "struct PointTraits<tachyon::math::%{type}::G1AffinePoint> {",
      "  using CPointTy = tachyon_%{type}_g1_point2;",
      "  using CCurvePointTy = tachyon_%{type}_g1_affine;",
      "  using CScalarField = tachyon_%{type}_fr;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon::math::%{type}::G1ProjectivePoint> {",
      "  using CPointTy = tachyon_%{type}_g1_point3;",
      "  using CCurvePointTy = tachyon_%{type}_g1_projective;",
      "  using CScalarField = tachyon_%{type}_fr;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon::math::%{type}::G1JacobianPoint> {",
      "  using CPointTy = tachyon_%{type}_g1_point3;",
      "  using CCurvePointTy = tachyon_%{type}_g1_jacobian;",
      "  using CScalarField = tachyon_%{type}_fr;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon::math::%{type}::G1PointXYZZ> {",
      "  using CPointTy = tachyon_%{type}_g1_point4;",
      "  using CCurvePointTy = tachyon_%{type}_g1_xyzz;",
      "  using CScalarField = tachyon_%{type}_fr;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_affine> {",
      "  using PointTy = tachyon::math::Point2<tachyon::math::%{type}::Fq>;",
      "  using CurvePointTy = tachyon::math::%{type}::G1AffinePoint;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_projective> {",
      "  using PointTy = tachyon::math::Point3<tachyon::math::%{type}::Fq>;",
      "  using CurvePointTy = tachyon::math::%{type}::G1ProjectivePoint;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_jacobian> {",
      "  using PointTy = tachyon::math::Point3<tachyon::math::%{type}::Fq>;",
      "  using CurvePointTy = tachyon::math::%{type}::G1JacobianPoint;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_xyzz> {",
      "  using PointTy = tachyon::math::Point3<tachyon::math::%{type}::Fq>;",
      "  using CurvePointTy = tachyon::math::%{type}::G1PointXYZZ;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_point2> {",
      "  using PointTy = tachyon::math::Point2<tachyon::math::%{type}::Fq>;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_point3> {",
      "  using PointTy = tachyon::math::Point3<tachyon::math::%{type}::Fq>;",
      "};",
      "",
      "template <>",
      "struct PointTraits<tachyon_%{type}_g1_point4> {",
      "  using PointTy = tachyon::math::Point4<tachyon::math::%{type}::Fq>;",
      "};",
      "",
      "template <>"
      "struct PrimeFieldTraits<tachyon_%{type}_fq> {",
      "  using PrimeFieldTy = tachyon::math::%{type}::Fq;",
      "};",
      "",
      "template <>"
      "struct PrimeFieldTraits<tachyon_%{type}_fr> {",
      "  using PrimeFieldTy = tachyon::math::%{type}::Fr;",
      "};",
      "",
      "}  // namespace tachyon::cc::math",
  };

  std::string tpl_content = absl::StrJoin(tpl, "\n");

  std::string content = absl::StrReplaceAll(
      tpl_content, {
                       {"%{header_dir_name}", GetLocation(type)},
                       {"%{fq_limb_nums}", absl::StrCat(fq_limb_nums)},
                       {"%{fr_limb_nums}", absl::StrCat(fr_limb_nums)},
                       {"%{type}", type},
                   });
  return WriteHdr(content);
}

int GenerationConfig::GenerateUtilHdr() const {
  // clang-format off
  std::vector<std::string_view> tpl = {
      "#include <ostream>",
      "",
      "#include \"tachyon/c/math/elliptic_curves/%{header_dir_name}/fr.h\"",
      "#include \"tachyon/c/math/elliptic_curves/%{header_dir_name}/g1.h\"",
      "",
      "%{insertion_ops}",
      "",
      "%{fq_comparison_ops}",
      "",
      "%{fr_comparison_ops}",
      "",
      "%{g1_affine_equality_ops}",
      "",
      "%{g1_projective_equality_ops}",
      "",
      "%{g1_jacobian_equality_ops}",
      "",
      "%{g1_xyzz_equality_ops}",
      "",
      "%{g1_point2_equality_ops}",
      "",
      "%{g1_point3_equality_ops}",
  };
  // clang-format on

  std::string tpl_content = absl::StrJoin(tpl, "\n");

  std::vector<std::string> insertion_ops_components;
  for (size_t i = 0; i < std::size(kPrimeFieldSuffices); ++i) {
    insertion_ops_components.push_back(
        build::GenerateInsertionOperatorDeclaration(
            absl::Substitute("tachyon_%{type}_$0", kPrimeFieldSuffices[i]),
            kPrimeFieldSuffices[i]));
    insertion_ops_components.push_back("");
  }
  for (size_t i = 0; i < std::size(kPointSuffices); ++i) {
    insertion_ops_components.push_back(
        build::GenerateInsertionOperatorDeclaration(
            absl::Substitute("tachyon_%{type}_$0", kPointSuffices[i]),
            "point"));
    if (i != std::size(kPointSuffices) - 1) {
      insertion_ops_components.push_back("");
    }
  }
  std::string insertion_ops = absl::StrJoin(insertion_ops_components, "\n");

#define DECLARE_COMPARISON_OPS(t)  \
  std::string t##_comparison_ops = \
      build::GenerateComparisonOpDeclarations("tachyon_%{type}_" #t)

  DECLARE_COMPARISON_OPS(fq);
  DECLARE_COMPARISON_OPS(fr);
#undef DECLARE_COMPARISON_OPS

#define DECLARE_EQUALITY_OPS(t)       \
  std::string g1_##t##_equality_ops = \
      build::GenerateEqualityOpDeclarations("tachyon_%{type}_g1_" #t)

  DECLARE_EQUALITY_OPS(affine);
  DECLARE_EQUALITY_OPS(projective);
  DECLARE_EQUALITY_OPS(jacobian);
  DECLARE_EQUALITY_OPS(xyzz);
  DECLARE_EQUALITY_OPS(point2);
  DECLARE_EQUALITY_OPS(point3);
#undef DECLARE_EQUALITY_OPS

  tpl_content = absl::StrReplaceAll(
      tpl_content,
      {
          {"%{insertion_ops}", insertion_ops},
          {"%{fq_comparison_ops}", fq_comparison_ops},
          {"%{fr_comparison_ops}", fr_comparison_ops},
          {"%{g1_affine_equality_ops}", g1_affine_equality_ops},
          {"%{g1_projective_equality_ops}", g1_projective_equality_ops},
          {"%{g1_jacobian_equality_ops}", g1_jacobian_equality_ops},
          {"%{g1_xyzz_equality_ops}", g1_xyzz_equality_ops},
          {"%{g1_point2_equality_ops}", g1_point2_equality_ops},
          {"%{g1_point3_equality_ops}", g1_point3_equality_ops},
      });

  std::string content = absl::StrReplaceAll(
      tpl_content, {
                       {"%{header_dir_name}", GetLocation(type)},
                       {"%{type}", type},
                   });
  return WriteHdr(content);
}

int GenerationConfig::GenerateUtilSrc() const {
  // clang-format off
  std::vector<std::string_view> tpl = {
      "#include \"tachyon/cc/math/finite_fields/prime_field_conversions.h\"",
      "#include \"tachyon/cc/math/elliptic_curves/point_conversions.h\"",
      "#include \"tachyon/cc/math/elliptic_curves/%{header_dir_name}/point_traits.h\"",
      "#include \"tachyon/math/elliptic_curves/%{header_dir_name}/g1.h\"",
      "",
      "using namespace tachyon::cc::math;",
      "",
      "%{insertion_ops}",
      "",
      "%{fq_comparison_ops}",
      "",
      "%{fr_comparison_ops}",
      "",
      "%{g1_affine_equality_ops}",
      "",
      "%{g1_projective_equality_ops}",
      "",
      "%{g1_jacobian_equality_ops}",
      "",
      "%{g1_xyzz_equality_ops}",
      "",
      "%{g1_point2_equality_ops}",
      "",
      "%{g1_point3_equality_ops}",
  };
  // clang-format on

  std::string tpl_content = absl::StrJoin(tpl, "\n");

  std::vector<std::string> insertion_ops_components;
  for (size_t i = 0; i < std::size(kPrimeFieldSuffices); ++i) {
    insertion_ops_components.push_back(
        build::GenerateInsertionOperatorDefinition(
            absl::Substitute("tachyon_%{type}_$0", kPrimeFieldSuffices[i]),
            kPrimeFieldSuffices[i],
            absl::Substitute("  return os << ToBigInt($0);",
                             kPrimeFieldSuffices[i])));
    insertion_ops_components.push_back("");
  }
  for (size_t i = 0; i < std::size(kPointSuffices); ++i) {
    insertion_ops_components.push_back(
        build::GenerateInsertionOperatorDefinition(
            absl::Substitute("tachyon_%{type}_$0", kPointSuffices[i]), "point",
            absl::Substitute("  return os << To$0(point);",
                             kInternalTypes[i])));
    if (i != std::size(kPointSuffices) - 1) {
      insertion_ops_components.push_back("");
    }
  }
  std::string insertion_ops = absl::StrJoin(insertion_ops_components, "\n");

#define DECLARE_COMPARISON_OPS(t)                                            \
  std::string t##_comparison_ops = build::GenerateComparisonOpDefinitions(   \
      "tachyon_%{type}_" #t, [](std::string_view op) {                       \
        return absl::Substitute("  return ToBigInt(a) $0 ToBigInt(b);", op); \
      })

  DECLARE_COMPARISON_OPS(fq);
  DECLARE_COMPARISON_OPS(fr);
#undef DECLARE_COMPARISON_OPS

#define DECLARE_EQUALITY_OPS(t, idx)                                        \
  std::string g1_##t##_equality_ops = build::GenerateEqualityOpDefinitions( \
      "tachyon_%{type}_g1_" #t, [](std::string_view op) {                   \
        return absl::Substitute("  return To$0(a) $1 To$0(b);",             \
                                kInternalTypes[idx], op);                   \
      })

  DECLARE_EQUALITY_OPS(affine, 0);
  DECLARE_EQUALITY_OPS(projective, 1);
  DECLARE_EQUALITY_OPS(jacobian, 2);
  DECLARE_EQUALITY_OPS(xyzz, 3);
  DECLARE_EQUALITY_OPS(point2, 4);
  DECLARE_EQUALITY_OPS(point3, 5);
#undef DECLARE_EQUALITY_OPS

  tpl_content = absl::StrReplaceAll(
      tpl_content,
      {
          {"%{insertion_ops}", insertion_ops},
          {"%{fq_comparison_ops}", fq_comparison_ops},
          {"%{fr_comparison_ops}", fr_comparison_ops},
          {"%{g1_affine_equality_ops}", g1_affine_equality_ops},
          {"%{g1_projective_equality_ops}", g1_projective_equality_ops},
          {"%{g1_jacobian_equality_ops}", g1_jacobian_equality_ops},
          {"%{g1_xyzz_equality_ops}", g1_xyzz_equality_ops},
          {"%{g1_point2_equality_ops}", g1_point2_equality_ops},
          {"%{g1_point3_equality_ops}", g1_point3_equality_ops},
      });

  std::string content = absl::StrReplaceAll(
      tpl_content, {
                       {"%{header_dir_name}", GetLocation(type)},
                       {"%{type}", type},
                   });
  return WriteSrc(content);
}

int RealMain(int argc, char** argv) {
  GenerationConfig config;

  base::FlagParser parser;
  parser.AddFlag<base::FilePathFlag>(&config.out)
      .set_long_name("--out")
      .set_help("path to output");
  parser.AddFlag<base::StringFlag>(&config.type)
      .set_long_name("--type")
      .set_required();
  parser.AddFlag<base::IntFlag>(&config.fq_limb_nums)
      .set_long_name("--fq_limb_nums")
      .set_required();
  parser.AddFlag<base::IntFlag>(&config.fr_limb_nums)
      .set_long_name("--fr_limb_nums")
      .set_required();

  std::string error;
  if (!parser.Parse(argc, argv, &error)) {
    tachyon_cerr << error << std::endl;
    return 1;
  }

  if (base::EndsWith(config.out.value(), "point_traits.h")) {
    return config.GeneratePointTraitsHdr();
  } else if (base::EndsWith(config.out.value(), "util.h")) {
    return config.GenerateUtilHdr();
  } else if (base::EndsWith(config.out.value(), "util.cc")) {
    return config.GenerateUtilSrc();
  } else {
    tachyon_cerr << "not supported suffix:" << config.out << std::endl;
    return 1;
  }
}

}  // namespace tachyon

int main(int argc, char** argv) { return tachyon::RealMain(argc, argv); }
