
#pragma once


#include <vector>
#include <unordered_map>
#include <string>

#include <gloperate/pipeline/Stage.h>


namespace gloperate
{


/**
*  @brief
*    Pipeline
*
*    A pipeline consists of several stages which are executed by the order
*    of their mutual dependencies. The pipeline itself is a stage and can
*    be used in the same way as any other stage, e.g., it can be part of
*    other pipelines.
*
*    The pipeline concept is that of a pull-pipeline:
*    - Output data can be marked as 'required'. Any stage will try to produce
*      all required output data, so if a stage has an output that is required
*      but invalid, it will be executed in order to produce that output.
*    - If output data is 'required', all input slots from that stage are also
*      marked as 'required'. This determines which stages will be executed
*      on a pipeline.
*    - When input data has changed, it will invalidate the respective outputs
*      of its stage, so any change of input data will propagate through the
*      pipeline immediately and invalidate all outputs that, directly or
*      indirectly, depend on that input.
*/
class GLOPERATE_API Pipeline : public Stage
{
    friend class Stage;


public:
    cppexpose::Signal<Stage *> stageAdded;   ///< Called when a new stage has been added
    cppexpose::Signal<Stage *> stageRemoved; ///< Called when a stage has been removed


public:
    /**
    *  @brief
    *    Constructor
    *
    *  @param[in] viewerContext
    *    Viewer context to which the stage belongs (must NOT be null!)
    *  @param[in] name
    *    Stage name
    *  @param[in] parent
    *    Parent pipeline (can be null)
    */
    Pipeline(ViewerContext * viewerContext, const std::string & name = "Pipeline", Pipeline * parent = nullptr);

    /**
    *  @brief
    *    Destructor
    */
    virtual ~Pipeline();

    /**
    *  @brief
    *    Get stages
    *
    *  @return
    *    List of stages in the pipeline
    */
    const std::vector<Stage *> stages() const;

    /**
    *  @brief
    *    Get stage by name
    *
    *  @param[in] name
    *    Name of stage
    *
    *  @return
    *    Stage (can be null)
    */
    Stage * stage(const std::string & name) const;

    // Virtual Stage interface
    virtual bool isPipeline() const override;


protected:
    /**
    *  @brief
    *    Register stage
    *
    *  @param[in] stage
    *    Stage (must NOT be null!)
    */
    void registerStage(Stage * stage);

    /**
    *  @brief
    *    Unregister stage
    *
    *  @param[in] stage
    *    Stage (must NOT be null!)
    */
    void unregisterStage(Stage * stage);

    // Virtual Stage interface
    virtual void onContextInit(AbstractGLContext * context) override;
    virtual void onContextDeinit(AbstractGLContext * context) override;
    virtual void onProcess(AbstractGLContext * context) override;
    virtual void onInputValueChanged(AbstractSlot * slot) override;
    virtual void onOutputRequiredChanged(AbstractSlot * slot) override;


protected:
    std::vector<Stage *>                     m_stages;    ///< List of stages in the pipeline
    std::unordered_map<std::string, Stage *> m_stagesMap; ///< Map of names -> stages
};


} // namespace gloperate
