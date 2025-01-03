export default class KNNClassifier {
  /** @type {number[][]} */
  data
  /** @type {Array<number|string>}*/
  labels

  /**
   * @param {number[][]} data
   * @param {Array<number|string>} labels
   */
  constructor(data, labels) {
    this.data = data
    this.labels = labels
  }

  /**
   * @param {number[]} point
   * @param {number=} k
   * @returns {{ classCounts: Record<string, number>, topClass: number|string, topClassCount: number }}
   */
  predict(point, k=3) {
    const kNearest = this.data
      .map((el, i) => ({
        dist: Math.hypot(...Object.keys(el).map(key => point[key] - el[key])),
        label: this.labels[i]
      }))
      .sort((a, b) => a.dist - b.dist)
      .slice(0, k)

    return kNearest.reduce(
      (acc, { label }) => {
        const current = acc.classCounts[label] + 1 || 1

        acc.classCounts[label] = current

        if (current > acc.topClassCount) {
          acc.topClassCount = current
          acc.topClass = label
        }
        return acc
      },
      {
        classCounts: {},
        topClass: kNearest[0].label,
        topClassCount: 0
      }
    )
  }
}
