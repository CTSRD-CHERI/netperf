@Library('ctsrd-jenkins-scripts') _

properties([disableConcurrentBuilds(),
            disableResume(),
            [$class: 'GithubProjectProperty', displayName: '', projectUrlStr: 'https://github.com/CTSRD-CHERI/cheri-netperf/'],
            [$class: 'CopyArtifactPermissionProperty', projectNames: '*'],
            [$class: 'JobPropertyImpl', throttle: [count: 3, durationName: 'hour', userBoost: true]],
            pipelineTriggers([githubPush()])
])

jobs = [:]

["mips-nocheri", "mips-hybrid", "mips-purecap"].each { suffix ->
    String name = "netperf-${suffix}"
    jobs[suffix] = { ->
        cheribuildProject(
            target: name,
            architecture: suffix,
            runTests:false,
            tarballName: "${name}.tar.xz")
    }
}

node("freebsd") {
    jobs.failFast = false
    parallel jobs
}