#include "VirtualReceiver.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace NavJest
{

namespace
{

constexpr double MetersPerKilometer = 1000.0;
constexpr int UnknownCount = 4;
constexpr int MaximumIterations = 12;
constexpr double PositionConvergenceMeters = 1.0e-4;
constexpr double ClockConvergenceMeters = 1.0e-4;

bool isFiniteVector(const OrbitMath::Vector3 &value)
{
    return std::isfinite(value.x)
        && std::isfinite(value.y)
        && std::isfinite(value.z);
}

bool solveLinearSystem(
    std::array<std::array<double, UnknownCount + 1>, UnknownCount> matrix,
    std::array<double, UnknownCount> &solution)
{
    for (int column = 0; column < UnknownCount; ++column)
    {
        int pivotRow = column;
        double pivotMagnitude = std::abs(matrix[column][column]);

        for (int row = column + 1; row < UnknownCount; ++row)
        {
            const double magnitude = std::abs(matrix[row][column]);

            if (magnitude > pivotMagnitude)
            {
                pivotMagnitude = magnitude;
                pivotRow = row;
            }
        }

        if (pivotMagnitude < 1.0e-12)
        {
            return false;
        }

        if (pivotRow != column)
        {
            std::swap(matrix[pivotRow], matrix[column]);
        }

        const double pivot = matrix[column][column];

        for (int item = column; item <= UnknownCount; ++item)
        {
            matrix[column][item] /= pivot;
        }

        for (int row = 0; row < UnknownCount; ++row)
        {
            if (row == column)
            {
                continue;
            }

            const double factor = matrix[row][column];

            for (int item = column; item <= UnknownCount; ++item)
            {
                matrix[row][item] -= factor * matrix[column][item];
            }
        }
    }

    for (int row = 0; row < UnknownCount; ++row)
    {
        solution[row] = matrix[row][UnknownCount];
    }

    return true;
}

double measurementWeight(double cn0DbHz)
{
    if (!std::isfinite(cn0DbHz))
    {
        return 1.0;
    }

    // 仅用于软件仿真的相对权重，限制范围以避免病态权重放大数值误差。
    const double rawWeight = std::pow(10.0, (cn0DbHz - 40.0) / 10.0);
    return std::max(0.1, std::min(10.0, rawWeight));
}

} // namespace

ReceiverSolution VirtualGnssReceiver::solvePosition(
    const std::vector<PseudorangeMeasurement> &measurements,
    const OrbitMath::Vector3 &initialPositionEcefKm) const
{
    ReceiverSolution result;

    if (measurements.size() < 4)
    {
        result.message = "至少需要 4 颗卫星的有效伪距观测量";
        return result;
    }

    if (!isFiniteVector(initialPositionEcefKm))
    {
        result.message = "接收机初始位置不是有效数值";
        return result;
    }

    std::vector<const PseudorangeMeasurement *> usable;
    usable.reserve(measurements.size());

    for (const PseudorangeMeasurement &measurement : measurements)
    {
        if (measurement.prn <= 0
            || !isFiniteVector(measurement.satelliteEcefKm)
            || !std::isfinite(measurement.pseudorangeMeters)
            || measurement.pseudorangeMeters <= 0.0)
        {
            continue;
        }

        usable.push_back(&measurement);
    }

    if (usable.size() < 4)
    {
        result.message = "有效伪距观测量不足 4 条";
        return result;
    }

    std::array<double, UnknownCount> state {
        initialPositionEcefKm.x * MetersPerKilometer,
        initialPositionEcefKm.y * MetersPerKilometer,
        initialPositionEcefKm.z * MetersPerKilometer,
        0.0
    };

    bool solved = false;

    for (int iteration = 0; iteration < MaximumIterations; ++iteration)
    {
        std::array<std::array<double, UnknownCount + 1>, UnknownCount> normal {};
        std::array<double, UnknownCount> rightHandSide {};

        for (const PseudorangeMeasurement *measurement : usable)
        {
            const double satelliteX =
                measurement->satelliteEcefKm.x * MetersPerKilometer;
            const double satelliteY =
                measurement->satelliteEcefKm.y * MetersPerKilometer;
            const double satelliteZ =
                measurement->satelliteEcefKm.z * MetersPerKilometer;

            const double dx = state[0] - satelliteX;
            const double dy = state[1] - satelliteY;
            const double dz = state[2] - satelliteZ;
            const double range = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (range < 1.0)
            {
                continue;
            }

            const std::array<double, UnknownCount> row {
                dx / range,
                dy / range,
                dz / range,
                1.0
            };

            const double predictedRange = range + state[3];
            const double residual =
                measurement->pseudorangeMeters - predictedRange;
            const double weight = measurementWeight(measurement->cn0DbHz);

            for (int rowIndex = 0; rowIndex < UnknownCount; ++rowIndex)
            {
                rightHandSide[rowIndex] +=
                    weight * row[rowIndex] * residual;

                for (int columnIndex = 0;
                     columnIndex < UnknownCount;
                     ++columnIndex)
                {
                    normal[rowIndex][columnIndex] +=
                        weight * row[rowIndex] * row[columnIndex];
                }
            }
        }

        for (int row = 0; row < UnknownCount; ++row)
        {
            normal[row][UnknownCount] = rightHandSide[row];
        }

        std::array<double, UnknownCount> correction {};

        if (!solveLinearSystem(normal, correction))
        {
            result.message = "卫星几何分布退化，最小二乘矩阵不可逆";
            return result;
        }

        state[0] += correction[0];
        state[1] += correction[1];
        state[2] += correction[2];
        state[3] += correction[3];

        result.iterations = iteration + 1;

        const double positionCorrection = std::sqrt(
            correction[0] * correction[0]
            + correction[1] * correction[1]
            + correction[2] * correction[2]);

        if (positionCorrection < PositionConvergenceMeters
            && std::abs(correction[3]) < ClockConvergenceMeters)
        {
            solved = true;
            break;
        }
    }

    double residualSquareSum = 0.0;
    int residualCount = 0;

    for (const PseudorangeMeasurement *measurement : usable)
    {
        const double dx = state[0]
            - measurement->satelliteEcefKm.x * MetersPerKilometer;
        const double dy = state[1]
            - measurement->satelliteEcefKm.y * MetersPerKilometer;
        const double dz = state[2]
            - measurement->satelliteEcefKm.z * MetersPerKilometer;
        const double range = std::sqrt(dx * dx + dy * dy + dz * dz);
        const double residual = measurement->pseudorangeMeters
            - (range + state[3]);

        residualSquareSum += residual * residual;
        ++residualCount;
    }

    result.positionEcefKm = {
        state[0] / MetersPerKilometer,
        state[1] / MetersPerKilometer,
        state[2] / MetersPerKilometer
    };
    result.clockBiasMeters = state[3];
    result.rmsResidualMeters = residualCount > 0
        ? std::sqrt(residualSquareSum / residualCount)
        : std::numeric_limits<double>::infinity();
    result.usedSatellites = static_cast<int>(usable.size());

    result.valid = isFiniteVector(result.positionEcefKm)
        && std::isfinite(result.clockBiasMeters)
        && std::isfinite(result.rmsResidualMeters);

    if (result.valid)
    {
        result.message = solved
            ? "最小二乘定位解算收敛"
            : "达到最大迭代次数，返回当前解算结果";
    }
    else
    {
        result.message = "定位解算结果包含无效数值";
    }

    return result;
}

} // namespace NavJest
